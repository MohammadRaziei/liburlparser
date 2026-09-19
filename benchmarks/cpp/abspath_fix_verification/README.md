# abspath() fix - verification tool

`fair_compare.cpp` is a single-process, interleaved A/B/C benchmark comparing:
- the *old* (unfixed) `abspath()` - recomputes '.'/'..' resolution from
  scratch on every call, allocating a `std::string` + `std::vector<size_t>`
  each time
- the *new* (fixed) `abspath()` - a fast pre-scan (`path_is_already_normalized`)
  skips that work entirely when the path has no dot-segments or repeated
  slashes (the common case), and the result is cached (`abspath_cache_`)
- `ada::url_aggregator`, unmodified, as an external reference point

All three are measured in the same process, in 12 interleaved rounds (old,
then new, then ada, repeated), so environment noise (scheduler jitter,
thermal throttling) is distributed roughly evenly across all three instead
of biasing whichever ran last - an earlier two-separate-process comparison
was invalidated by exactly this kind of noise.

The actual fix lives directly in `include/urlparser.h` and
`src/urlparser.cpp` (the `path_is_already_normalized()` fast path and the
`abspath_cache_` member) - no separate patch file, it's just part of the
normal source.

## Results (this machine, 3 runs)

```
liburlparser OLD (unfixed abspath):   ~5.4-5.5M ops/s
liburlparser NEW (fixed abspath):     ~7.4-7.8M ops/s
ada:                                  ~6.0-6.1M ops/s

new vs old: ~1.37-1.41x
new vs ada: ~1.24-1.29x  (liburlparser now faster than ada)
old vs ada: ~0.90-0.91x  (liburlparser was slower than ada before the fix)
```

## Build & run

Needs a local build of liburlparser (both the original and patched source,
each compiled under a renamed namespace so they can coexist in one binary)
and ada. See the `_old`/`_new` namespace-rename trick used to produce
`urlparser_old.h`/`.cpp` from the unpatched source - not included here
since it's a one-off scratch step, not part of the library itself.
