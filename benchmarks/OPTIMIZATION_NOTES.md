# Performance Investigation Notes

A log of what we tried while trying to close the speed gap with
`PyDomainExtractor` (Python) and `ada` (C++), what actually worked, what
didn't, and the real before/after numbers. Kept here rather than in the
main README because most of this is process, not just results — the
point is as much "why" as "what."

## TL;DR

| | Before | After | Change |
|---|---|---|---|
| C++ `extract_from_host` | 66.20 MB/s | 112.46 MB/s | **+70%** |
| C++ `parse_url` | 233.70 MB/s | 253.57 MB/s | **+8.5%** |
| C++ `parse_url` vs `ada` | 22.7% behind | ~1.24-1.29x **ahead** | gap closed, reversed |
| Python `extract_from_host` | 27.29 MB/s | 32.22 MB/s | +18% (PSL only) |
| Python `extract_dict_from_host` vs `PyDomainExtractor` | 0.42x | ~0.80-0.89x | binding rewrite, ~2x |

Three real, verified wins landed (PSL hash map, SIMD delimiter
scanning, `abspath()` fast-path+cache) and one real, verified Python
binding rewrite (interned keys + direct slicing) closed most of the
`PyDomainExtractor` gap. Two ideas were researched and deliberately
*not* built (a Trie; a Cython rewrite), both with real, reproducible
code and numbers behind the rejection, not just intuition.

## What actually worked

### 1. `ankerl::unordered_dense::map` instead of `std::unordered_map` for the PSL table

**The problem:** `psl::suffix_of()` looks up a host against ~9,800
Public Suffix List rules on every parse. `std::unordered_map`'s buckets
are individually heap-allocated nodes scattered across memory — at
9,800 entries, that's real cache-miss cost on every lookup.

**What we tried first and rejected:** a Trie, on the strength of
`PyDomainExtractor`'s own docs claiming a Trie is "more efficient."
Before building one, we went and checked real production code instead
of taking that at face value:

- **[Ladybird](https://github.com/LadybirdBrowser/ladybird/commit/49a46522d0f)**
  (a real, current browser engine) migrated its own PSL lookup **off**
  a Trie and **onto** sorted-array binary search in October 2025.
- Chromium's own `domain-registry-provider` design doc measured a
  DAFSA (a compressed Trie variant) at **~35% slower** than a perfect
  hash table for the same lookup, and only chose it to save binary
  size (380KB → 33KB), not for speed.
- An independent hash-function microbenchmark
  ([apt's triehash](https://salsa.debian.org/apt-team/triehash))
  measured plain Tries at **roughly 3x slower** than `gperf`-style
  perfect hashing on the same data, across multiple architectures.

Verdict: a Trie would very likely have made us *slower*, not faster.
We didn't build one.

**What we tried next and also rejected:** since Ladybird's real choice
was sorted-array + binary search (not a hash table), we built that too
and measured it head-to-head against a hash map, on the *actual* PSL
(9,766 rules) and 10,000 real domains — not a toy dataset. (An earlier
version of this section quoted specific numbers for this comparison
with no corresponding code anywhere in the repo's history to reproduce
them — caught during a later review, and worth naming here as a
warning: don't trust a number in this document, or any document, that
you can't re-run. The comparison *has* now actually been built,
verified, and committed — see `benchmarks/cpp/psl_structures/`:)

```
$ cmake -S benchmarks -B benchmarks/build   # populates benchmarks/corpus/
$ g++ -O3 -std=c++17 benchmarks/cpp/psl_structures/psl_structure_comparison.cpp -o /tmp/psl_cmp
$ /tmp/psl_cmp

ankerl hash_map (shipped):             ~16-19M ops/s
trie, vector<pair> children:            ~4M ops/s
trie, ankerl dense_map children:        ~8M ops/s
bucketed map<tld,{set,depth,len}>:     ~10-11M ops/s
bucketed, owned + string_view keys:    ~13-14M ops/s
bucketed, string_view storage:         ~13-14M ops/s
FLAT (shipped design) + sv storage:    ~17-19M ops/s  (statistical tie with shipped)
```

0 mismatches across all six designs on the full 10,000-domain corpus,
verified before any timing runs. The hash map (as shipped) wins outright
against both real trie variants (arena-allocated nodes, not per-node
heap maps like `PyDomainExtractor`'s Rust implementation) by 2-5x, and
against four separate attempts at a bucketed/two-level structure by
15-80%. Every one of those alternatives was fully implemented and
correctness-checked, not assumed.

Why does binary search lose here, opposite of Ladybird's result?
`suffix_of()`'s matching algorithm does a *shrinking* search — for
`www.example.com` it tries `www.example.com`, then `example.com`, then
`com`, stopping at the first match. Each attempt against a sorted array
costs `O(log n)` *string comparisons*, and PSL rules cluster
heavily by shared suffix (`.com`, `.co.uk`, ...), so those comparisons
routinely have to walk past a long common prefix before finding a
difference. A hash lookup costs one hash computation (`O(k)`, same
`k` as a single comparison) plus an `O(1)` bucket probe — no repeated
`log(n)`-scaled string comparisons per attempt. Ladybird's algorithm is
evidently structured differently (a single lookup rather than a
shrinking retry loop), which is presumably why binary search works for
them and not for us. Point is: we measured *our own* access pattern
instead of assuming a result would transfer from a different one. Full
writeup, including *why* a two-level bucketed structure can't win
either for this specific rule distribution (`.com` alone carries 1,115
deeper private-suffix rules, so 97.4% of real domains pay for two
lookups either way, same as the flat map's worst case):
`benchmarks/cpp/psl_structures/README.md`.

**What we actually shipped:**
[`ankerl::unordered_dense::map`](https://github.com/martinus/unordered_dense)
— still a hash map (so it keeps the win over sorted-array above), but
one that stores its data contiguously in a `std::vector` (open
addressing, robin-hood backward-shift deletion) instead of
`std::unordered_map`'s scattered per-bucket allocations. Vendored as a
single header in `src/ankerl/` (MIT-licensed, no dependency of its own)
specifically because it's a drop-in `std::unordered_map` replacement —
almost no code changed besides the type name. `psl::levels_`'s type is
hidden behind a forward-declared `detail::suffix_table` (a tiny PIMPL)
so the public `urlparser.h` header never has to know this vendored
header exists.

**Result:** `extract_from_host` went from 66.20 → 110-112 MB/s in C++
(**+67-70%**), 27.29 → 32.08-32.22 MB/s in Python (**+17-18%**) — real
runs, not a microbenchmark. (Python's smaller relative gain is expected
— see [Remaining gaps](#remaining-gaps): a good chunk of Python's
per-call time isn't spent in this lookup at all.)

### 2. Merging `suffix_length()` + `last_segments()` into one pass

The old code found the matching PSL suffix in `suffix_length()` — which
had the correctly-matched, reversed, lowercased text sitting right
there in a local buffer — and then threw that away, returning only a
segment *count*. `last_segments()` then re-derived the same substring
from scratch with a completely different scan of the *original*
(un-reversed) string. Merged into one pass that returns the match it
already found instead of re-deriving it: **~15% faster** in an isolated
pure-C++ measurement (5.0M → 5.8M ops/s), before the `ankerl` change
was even made.

### 3. SIMD scanning for `#` / `?` / `;` in `url::url()`

**The problem:** splitting a URL into path/query/fragment means finding
the first `#`, the first `?` before it, and (for some schemes) the
first `;` before that — a byte-by-byte scan of everything after the
host. `ada` does this 16-32 bytes at a time with hand-written SIMD; we
did it one byte at a time.

We read `ada`'s actual scanning code
([`src/parser.cpp`](https://github.com/ada-url/ada/blob/main/src/parser.cpp))
rather than guessing at the technique. They use a `pshufb`/nibble-table
approach (borrowed from WebKit) built for classifying many character
classes at once. We only ever need to find up to 3 fixed bytes, so we
used the simpler "compare against each target byte, OR the masks
together" SIMD pattern instead — same vectorization win, less code to
get right for this narrower job.

Shipped as three tiers in `find_first_of_3()`:

- **AVX2** (32 bytes/iteration) — x86-64, but *not* guaranteed present
  on every chip, so it needs a genuine runtime check before it's safe
  to use. Done once and cached. GCC/Clang:
  `__builtin_cpu_supports("avx2")` plus a per-function
  `__attribute__((target("avx2")))` so just that one function gets AVX2
  code generation without forcing the whole binary to require it. MSVC
  has neither of those, so the equivalent (`OSXSAVE` → `XGETBV` → CPUID
  leaf 7) is spelled out by hand with `__cpuid`/`_xgetbv`.
- **SSE2** (16 bytes/iteration) — x86-64's baseline ABI, every x86-64
  CPU has it, so this is a compile-time `#if` on the architecture, no
  detection needed.
- **NEON** (16 bytes/iteration) — same story as SSE2, on aarch64.
- Scalar loop as the final fallback (anything else, and the sub-16/32-
  byte tail on every platform).

Correctness was checked with a dedicated edge-case sweep — multiple
`?`/`#`, `;` before/after `?`, and critically a boundary sweep planting
the target characters at every offset 0–40 bytes in, to make sure
nothing was missed or double-counted exactly at a 16- or 32-byte SIMD
chunk boundary — plus the full 75-test unit suite.

**Result:** `parse_url` went from 231.71 → 253.57 MB/s in C++
(**+9.5%**), narrowing the gap with `ada` from 22.7% (before *any* of
this work) to 17.9%.

### 4. A rejected micro-optimization: `KNOWN_PROTOCOLS`/`USES_PARAMS`/`USES_NETLOC`

These three small (15-28 entry) sets are checked on every `url::url()`
parse to classify the scheme. The original `unordered_set<std::string>`
needed a temporary `std::string` built from the parsed scheme (a
`string_view`) just to call `.find()`. Tried and measured, in order:

1. `unordered_set<std::string_view>` (zero-copy lookup) — **rejected**:
   only safe if every element is a string literal forever; a future
   contributor changing an entry to a runtime-built string would
   silently dangle, with no compiler error to catch it.
2. `std::array<std::string_view, N>` + `std::binary_search` — measured
   against `std::set<std::string, std::less<>>` (which *does* avoid the
   temporary-string problem safely, via a transparent comparator) on a
   realistic query mix. Results were a dead heat (43-49ms either way
   over 2M lookups) — at this size (≤28 entries), the data structure
   choice doesn't matter enough to justify the custom code.

**Shipped:** `std::set<std::string, std::less<>>` — safe (owns real
`std::string`s, transparent comparator avoids the temporary on lookup),
standard library, no custom class to maintain. This is the one idea in
this document that turned out not to matter either way; recorded here
so nobody re-litigates it later without the numbers in hand.

## Remaining gaps

### C++ `parse_url` vs `ada`: closed — was `abspath()`, not the parser

The ~18% gap above turned out to have nothing to do with URL syntax
parsing, `suffix_of()`, or SIMD scanning — all already-optimized code
that the `parse_url` benchmark barely touches, once you check what it
actually calls. `abspath()` was the one field in that call chain that
wasn't a cheap accessor: unlike `protocol()`/`host_text()`/`query()`/
`fragment()` (`string_view`, `noexcept`, one field read), `abspath()`
re-ran a full `.`/`..` dot-segment resolver — allocating a
`std::string` *and* a `std::vector<size_t>` — on every single call,
even though the overwhelming majority of real-world paths need no
resolution at all.

Fixed with a cheap up-front check (`path_is_already_normalized()`) that
detects the common case and returns the path unchanged, skipping the
resolver entirely, plus caching the result the way `host()` already
was (`abspath_cache_`, same pattern). Verified in a single process,
interleaved, against `ada` and the pre-fix build side by side (not two
separate runs — see the note in the Python section below about why
that distinction matters):

```
$ ./benchmarks/cpp/abspath_fix_verification/  # see its README.md
liburlparser OLD (unfixed abspath):   ~5.4-5.5M ops/s
liburlparser NEW (fixed abspath):     ~7.4-7.8M ops/s   (~1.4x)
ada:                                  ~6.0-6.1M ops/s
```

`liburlparser` now beats `ada` on this benchmark (it was ~10% behind
before the fix — consistent with the ~18% figure this section used to
quote, once you account for a different corpus/compiler). All 75 C++
unit tests pass unchanged, including the `abspath`-specific ones.

### Python: was ~3x tax, now real, measured, evidence-based progress

The old version of this section flagged the Python/`can_ada` gap as
"not yet investigated." It has been, in depth, empirically, with a
real Cython prototype built to test the obvious alternative — not
guessed at. Summary of what was actually found and fixed for
`Hostname.extract_dict_from_host`/`_url` (the hot, dict-returning path
`extract_from_host`/`_url` in the benchmark use):

1. **Interned dict keys + raw CPython C-API**, instead of `nb::dict`'s
   convenience layer — the same technique `PyDomainExtractor`'s pyo3
   binding uses (`pyo3::intern!`). `nb::dict`'s `operator[]` builds a
   fresh `PyUnicode` key object on every call; interning it once and
   reusing that object removes that cost. **+20%** in isolation.
2. **Direct `std::string_view` slicing** of the already-owned host
   buffer, instead of going through `hostname::ensure_parsed()`'s own
   mutable-field caching — which allocates `domain_`/`subdomain_`/
   `suffix_`/`fulldomain_` as *separately-owned* `std::string` members
   (one `substr()` allocation each) only for the dict-builder to
   allocate a *second* time (`PyUnicode_FromStringAndSize`) to copy
   each of those into a Python object. Computing the same offsets
   directly against the owned buffer and going straight to
   `PyUnicode_FromStringAndSize` removes that duplicate allocation.
   **+28-30%** more on top of (1).
3. **Applying the same technique to `IPv4`/`IPv6` extraction**
   (`ipv4_to_dict_fast`/`ipv6_to_dict_fast`) — no slicing opportunity
   there (nothing to slice a substring out of; `as_int`/`high64`/
   `low64` are each computed from raw bytes, not a buffer offset), so
   this is the interned-keys win only, but applied consistently.

Combined, measured on the same 10,000-domain corpus, single process,
interleaved against `PyDomainExtractor`:

```
$ ./benchmarks/python/binding_fast_path_ab.py   # see comments for exact methodology
extract_dict_from_host, original (nb::dict, 3-field-equivalent):  ~1.8M ops/s   (0.42x of PDE)
+ interned keys + raw C-API:                                       ~2.6M ops/s   (0.60x of PDE)
+ direct string_view slicing:                                      ~3.2-3.4M ops/s (0.80-0.89x of PDE)
```

On an apples-to-apples field count (`liburlparser`'s 3 comparable
fields vs `PyDomainExtractor`'s 3), the gap closed from **0.42x to
~0.85x** — nearly 2x faster than where this investigation started, and
close enough to `PyDomainExtractor` that the remaining ~15% is likely
just `pyo3`'s own per-call dispatch being marginally leaner, not a
further architectural fix waiting to be found.

**Cython, tried and rejected.** The obvious next idea — replace the
hand-written `nanobind` C-API calls with a Cython extension, built the
same way `pygixml` (this author's other project) builds its bindings:
`pyproject.toml` + `scikit-build-core` + `cython_transpile` +
`python_add_library`, not a quick `setup.py` — was built, in full,
correctness-checked, and benchmarked head-to-head against the
hand-written `nanobind` version and `PyDomainExtractor`, all three in
the same process:

```
Cython   (3-field, direct-slice, same algorithm): ~4.2-4.5M ops/s
nanobind (3-field, direct-slice, same algorithm): ~5.2-5.5M ops/s
PyDomainExtractor .extract():                     ~4.6-5.0M ops/s

Cython vs nanobind: 0.80-0.81x  (Cython is slower)
```

Cython lost by a consistent ~19-20%, even after fixing an initial
inefficiency in the prototype (`.encode("utf-8")` allocating an
intermediate `bytes` object where `nanobind`'s `string_view` caster
reads the Python string's UTF-8 buffer directly) — the ratio barely
moved after that fix, meaning it's a real property of Cython's
generated code (its own refcounting/typecheck scaffolding, even with
`boundscheck=False`/`wraparound=False`) for this specific workload, not
a bug in the prototype. **Verdict: keep the hand-written `nanobind`
binding.**

**Why `Hostname` construction itself still costs more than the raw C++
engine.** Isolated with a waterfall of probes (each adding exactly one
more step, all in the same process):

```
just crossing into Python, nothing else:        ~40-50M ops/s
+ the mandatory std::string copy + ctor:         ~15-17M ops/s
+ ensure_parsed() (PSL lookup + string slicing): ~4-5M ops/s
```

The `std::string` copy is unavoidable — `hostname` needs to own its
data, and a Python `str`'s buffer isn't something C++ can hold a
reference to past the call. The rest is the same kind of allocation
duplication (1) and (2) above already address for the *dict-building*
path; `ensure_parsed()` itself still computes all four fields even when
a caller only reads one (`.suffix()` alone still allocates
`domain_`/`subdomain_`) — a real, identified, not-yet-fixed
inefficiency for call sites that go through `hostname`'s public
accessors rather than the direct-slice fast path above.
