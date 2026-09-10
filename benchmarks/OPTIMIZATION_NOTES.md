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
| C++ `parse_url` gap vs `ada` | 22.7% | 17.9% | narrowed |
| Python `extract_from_host` | 27.29 MB/s | 32.22 MB/s | **+18%** |
| Python `extract_from_url` | 68.49 MB/s | 77.43 MB/s | **+13%** |

Two real, verified wins landed (PSL hash map, SIMD delimiter scanning).
One idea (a Trie) was researched and deliberately *not* built, with
evidence for why. One idea (sorted-array binary search) was actually
built and measured, and lost decisively to what we already had. Python
still trails `PyDomainExtractor` and `can_ada` by a wider margin than
the raw C++ engine does — see [Remaining gaps](#remaining-gaps).

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
(9,766 rules) and 10,000 real domains — not a toy dataset:

```
ankerl hash_map:          18.8M ops/s
sorted_vec+binary_search:  3.6M ops/s
```

The hash map won by **~5x**. Why the opposite of Ladybird's result?
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
instead of assuming a result would transfer from a different one.

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

### C++ `parse_url` vs `ada`: ~18%

Down from ~23%, but not closed. The rest is very likely `ada`'s
`pshufb`/nibble-table SIMD technique (built for classifying dozens of
character classes at once, not just finding 3 fixed bytes) plus years
of the kind of micro-tuning a dedicated team applies to a
widely-embedded (Node.js, Cloudflare Workers, ClickHouse) parser.
Closing the rest would mean matching that technique, not another
targeted fix like the ones above.

### Python: ~3x tax between raw C++ and what Python actually sees

Measured directly: `urlparser::hostname` in pure C++ does ~8.47M
ops/s; the *exact same code* through the Python binding manages ~2.7M
— a `~3.1x` overhead ratio that has nothing to do with the PSL/parsing
algorithm (already fast) and everything to do with the Python/C++
boundary itself (constructing a wrapped object, marshaling
`std::string` ↔ `str`, normal CPython attribute-access overhead).

This is why Python's relative gains from the `ankerl` switch (+17-18%)
are smaller than C++'s (+67-70%) — the win is real, but it's a smaller
slice of a total that's now mostly binding overhead, not lookup time.

This also shows up in the freshest data point we have,
`liburlparser` vs `can_ada` on `parse_url` **in Python**:

```
liburlparser   parse_url    42.25 MB/s   1,062,177 ops/s
can_ada        parse_url    64.73 MB/s   1,627,442 ops/s
```

A **35% gap** — wider than the 18% gap the same two engines show in
C++, consistent with `can_ada`'s own binding (pybind11, wrapping the
same fast `ada` core) simply carrying less relative overhead per call,
or liburlparser's specific binding pattern (constructing a wrapped
`Hostname`/`Url` object, then reading a property, vs a single call
returning already-marshaled data) costing more than it needs to.

Not yet investigated. `to_dict()` was tried once, on the
`extract_from_host` side, as a "return everything in one call" test —
it was *slower* than construct-then-read-one-property, not faster, so
the fix (if there is one that doesn't just move the cost around) isn't
obvious and needs its own real profiling pass rather than another
guess.
