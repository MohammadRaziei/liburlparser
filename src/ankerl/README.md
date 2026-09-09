# ankerl::unordered_dense

Vendored from https://github.com/martinus/unordered_dense (v4.8.1, MIT
license — see LICENSE in this folder).

**Why:** `urlparser::psl` looks up ~9,000 Public Suffix List rules on
every hostname/URL parse. `std::unordered_map`'s buckets are individually
heap-allocated nodes, scattered across memory — that's cache-unfriendly at
this scale. `ankerl::unordered_dense::map` stores its data contiguously in
a `std::vector` instead (open addressing, robin-hood backward-shift
deletion), which is the same reason a real browser engine
([Ladybird](https://github.com/LadybirdBrowser/ladybird/commit/49a46522d0f))
moved its own PSL lookup off a Trie and onto a flat, cache-friendly
structure. It's a near-drop-in replacement for `std::unordered_map` (same
`.find()`/`operator[]`/iterator API), single-header, and has no
dependency of its own — that's why it's vendored here as one file rather
than pulled in via CMake `FetchContent`: `psl` loads lazily on first
parse, adding a build-system dependency for a single header would be
disproportionate to what it's for.

**Do not hand-edit `unordered_dense.h`.** To update, re-download the
`include/ankerl/unordered_dense.h` file from a tagged release of the
upstream repository and replace it wholesale.
