# PSL data-structure comparison (reproducible)

`psl_structure_comparison.cpp` is a standalone, self-contained head-to-head
benchmark of six candidate designs for `psl::suffix_of()`'s lookup table,
built against the project's *actual* `public_suffix_list.dat` (~9,766
rules) and a bundled sample of 10,000 real-world domains
(`domains.txt`). It exists because an earlier version of
`../../OPTIMIZATION_NOTES.md` asserted specific benchmark numbers for a
Trie and a sorted-array/binary-search design with **no corresponding code
anywhere in the repo's history** to reproduce them. Those numbers may or
may not have been accurate - there was no way to check. This file is the
fix: every number quoted in the docs about this topic should trace back to
a run of this file.

## What it compares

1. **`ankerl hash_map`** - the design actually shipped in `src/urlparser.cpp`
   (`suffix_table`): one flat `ankerl::unordered_dense::map<string,size_t>`,
   reversed-string keys, a shrinking search bounded by `max_length`/`max_depth`.
2. **Trie, `vector<pair>` children** - arena-allocated nodes, sorted-vector
   children, `binary_search`.
3. **Trie, `ankerl dense_map` children** - same arena, but each node's
   children are an `ankerl::unordered_dense::map` instead of a vector.
4. **Bucketed `map<tld, {set, max_depth, max_length}>`** - outer map keyed
   by the TLD label, each bucket holding its own `ankerl::unordered_dense::set`
   of deeper suffixes plus its own (tighter, per-TLD) `max_length`/`max_depth`.
5. **Same, but the hostname is taken by value and reversed in place**, so
   every slice into it downstream is a `string_view` instead of a fresh
   `substr()` copy (only the input parameter itself is owned/copied).
6. **Same again, but the *storage* side is also `string_view`**: every PSL
   rule's reversed bytes live in one contiguous owned buffer
   (`suffix_table_sv::psl_text`), and every map/set key is a `string_view`
   slice into it - no per-rule `std::string` at all.

All six are checked for byte-identical output against every domain in
`domains.txt` before any timing happens; the run aborts loudly (prints up
to 10 mismatches) if any of them disagree.

## Results (this machine, `-O3`, single run - re-run for your own numbers)

```
ankerl hash_map (shipped):             ~16-19M ops/s
trie, vector<pair> children:            ~4M ops/s
trie, ankerl dense_map children:        ~8M ops/s
bucketed map<tld,{set,depth,len}>:     ~10-11M ops/s
bucketed, owned + string_view keys:    ~13-14M ops/s
bucketed, string_view storage (v3):    ~13-14M ops/s
```

The shipped flat hash map wins outright. The two trie variants lose by
2-5x. The bucketed variants close the gap a long way (successive
optimization: plain copies -> owned+view -> string_view storage) but
plateau at roughly **75-85% of the shipped map's throughput** and never
cross it.

**Why bucketing can't win here:** the idea was that the second (per-TLD)
search should be over a much smaller set, so it should be cheaper. It
doesn't pan out, because:
- A hash lookup's cost is `O(1)` in the *number of probes*, essentially
  independent of table size - shrinking the table doesn't shrink the cost
  of an individual lookup by much.
- The real saving would come from *skipping* the second lookup entirely
  (when a TLD's bucket has no deeper rules). But `.com` alone carries
  **1,115** deeper (private-suffix) rules - blogspot, wixsite, herokuapp,
  amazonaws, and hundreds more - and `.com` dominates real-world traffic.
  Measured on the bundled 10,000-domain sample: only **2.6%** of lookups
  hit a bucket with an empty `rest` (the one-lookup fast path); **97.4%**
  need both the outer and the inner lookup - the same total lookup count
  as the shipped flat map's shrinking search, plus the bookkeeping
  overhead of two separate container types instead of one repeated loop.

Switching the *storage* side to `string_view` (variant 6) made no further
difference over variant 5 (ratio ~1.0x) - most PSL entries are short
enough to already live inline via `std::string`'s short-string
optimization, so there was no per-rule heap allocation left to remove.

## Build & run

From the repo root (paths default to the repo layout; pass your own if
running from elsewhere):

```sh
g++ -O3 -std=c++17 benchmarks/cpp/psl_structures/psl_structure_comparison.cpp \
    -o /tmp/psl_structure_comparison
/tmp/psl_structure_comparison
# or: /tmp/psl_structure_comparison path/to/public_suffix_list.dat path/to/domains.txt
```

No external dependencies beyond the C++17 standard library and the
project's own vendored `src/ankerl/unordered_dense.h`.
