# liburlparser Benchmarks

Throughput benchmarks for liburlparser's C++ core and Python bindings.
Every language runs alongside every maintained competing implementation
that exists for it. This project has **no relationship to the repository
root**, not even for liburlparser itself: both languages fetch
liburlparser the exact same way they fetch any competitor — from
`https://github.com/mohammadraziei/liburlparser.git` (C++, via
`FetchContent`) or `pip install git+https://github.com/mohammadraziei/
liburlparser.git` (Python). **liburlparser is never a special case
here** — it's one more row in the same results table as `ada` or
`tldextract`.

**[OPTIMIZATION_NOTES.md](OPTIMIZATION_NOTES.md)** has the story behind
the current numbers: what was tried, what actually worked, what was
measured and rejected (including why a Trie for the PSL table turned
out to be a bad idea despite a competitor's docs recommending one), and
what gaps are still open.

## Running

`benchmarks/` is a self-contained world of its own — nothing here reads
or references anything outside this folder, so you `cd` into it first
and run everything from there:

```bash
cd benchmarks
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target urlparser_benchmarks
```

That fetches every dependency (the corpus, and each language's own
libraries) and runs every benchmark whose language toolchain is
present. To run just one language:

```bash
cmake --build build-bench --target urlparser_benchmarks_cpp
cmake --build build-bench --target urlparser_benchmarks_python
```

Building the top-level `urlparser_benchmarks` target (or the dedicated
`urlparser_benchmarks_report` target on its own once both languages have
run at least once) also renders a single, self-contained
**`results/report.html`** — Chart.js and every result are embedded
directly in the file, so it needs nothing else (no server, no network,
no sibling files) to open and read. That's the one artifact from this
whole directory meant to be committed; everything else here (the venv,
the per-language JSON, the fetched corpus, Chart.js itself) is
disposable build output and stays out of git (see `.gitignore`).

Each one prints a results table and writes a JSON file to
`build-bench/results/<language>.json` (a *disposable build-dir* results
folder — not the source-tree `benchmarks/results/` that holds the final
`report.html`; see "Layout" below):

```json
{
  "language": "python",
  "corpus": {"domains": 10000, "urls": 10000},
  "results": [
    {"library": "liburlparser", "operation": "extract_from_host",
     "throughput_mb_s": 23.65, "ops_per_sec": 2008747.0,
     "success_rate": 1.0, "total_time_s": 0.0996},
    ...
  ]
}
```

The JSON files are kept **separate per language** rather than merged —
each language tests a different set of libraries and has its own
loading overhead, so there's no meaningful single "language-agnostic"
number to combine them into.

## Layout

Mirrors `tests/`, one folder per language — each fetches its own
dependencies (including liburlparser) and defines an
`urlparser_benchmarks_<lang>` target:

```
benchmarks/
  CMakeLists.txt   orchestrator: fetches the corpus, detects toolchains,
                   adds each language below if its toolchain is present,
                   then renders the report
  cmake/           Download.cmake — generic file(DOWNLOAD) helper used
                   by report/ to fetch Chart.js
  cpp/             bench.cpp   — liburlparser, ada
  python/          bench.py    — liburlparser, tldextract, PyDomainExtractor,
                                  tld, publicsuffix2, can_ada, ada_url,
                                  giturlparse
  report/          generate_report.py + template.html.jinja2 — combines
                   both languages' JSON into results/report.html
  results/         report.html only — the one committed artifact; every
                   other input to it (JSON, Chart.js, the venv) lives in
                   the disposable build dir instead
```

## Methodology

Both benchmarks follow the same shape over the same corpus (a real
domain list fetched once by the top-level `CMakeLists.txt`):

1. **Load** the corpus into memory. Not timed.
2. **Timed — `extract_from_host`**: repeatedly extract the
   domain/suffix from a bare host string (no scheme, no path) — the
   same comparison already published in the project README, `x20` over
   the whole corpus.
3. **Timed — `extract_from_url`** (C++ calls this `parse_url`, and it's
   the only operation `ada`/`can_ada` appear in — see below): repeatedly
   parse a full URL (scheme + host + path + query + fragment) and
   extract every component, `x20` over the whole corpus.
4. **Report**: throughput in MB/s (of bytes actually read by
   *successful* extractions only) and operations/second, plus a
   success rate.

Not every library supports both operations — that's reported honestly
rather than papered over:

- **ada** (C++) and **can_ada** (Python, pybind11 bindings for that same
  ada-url engine) are pure WHATWG URL parsers with no Public Suffix List
  / domain-extraction feature, so they only have a `parse_url` row.
- **publicsuffix2** (Python) has no URL-aware entry point, so it only
  has an `extract_from_host` row.

A library that's missing an operation isn't penalized with a fabricated
zero — it simply doesn't get a row for that operation, the same
treatment ctoon's own benchmarks give a language with no competing
implementation.

## Corpus

**[felmoltor/robotstxt](https://github.com/felmoltor/robotstxt)**'s
`top.10000.majestic.domains.txt` — 10,000 real, currently-registered
domains (Majestic Million–derived), covering a wide range of TLDs,
ccTLDs, and subdomain shapes. Not synthetic data generated for this
benchmark.

The URL corpus is derived from those same domains: every third domain
gets a `www.` subdomain, and each one is wrapped in one of four
URL shapes (bare path, `?query`, nested path, or `#fragment`) so
`extract_from_url` / `parse_url` isn't just "scheme + bare host" for
every row.

Fetched via `file(DOWNLOAD ...)` at configure time (cached in
`build-bench/domains.txt` / `build-bench/urls.txt`) — nothing is
vendored into this repo.

## Competitors

**C++** — no other library benchmarked here does Public Suffix List
domain extraction (liburlparser's own specialty), so `extract_from_host`
is solo. For general URL parsing, liburlparser runs alongside:

- **[ada](https://github.com/ada-url/ada)** — WHATWG-compliant URL
  parser used in Node.js, Cloudflare Workers, ClickHouse, and Redpanda.

**Python** — the same four competitors already benchmarked in this
project's own README, now run through the same harness as every other
language here:

- **[tldextract](https://github.com/john-kurkowski/tldextract)**
- **[PyDomainExtractor](https://github.com/Intsights/PyDomainExtractor)**
  (Rust-backed)
- **[tld](https://github.com/barseghyanartur/tld)**
- **[publicsuffix2](https://github.com/aboutcode-org/python-publicsuffix2)**
- **[can_ada](https://github.com/TkTech/can_ada)** — pybind11 bindings
  for the same `ada` C++ engine benchmarked on the C++ side.

## Toolchain detection

A language is **skipped with a warning** only when its toolchain itself
isn't found:

- C++ always runs — this project's own `CMakeLists.txt` requires a CXX
  compiler just to configure, so there's nothing to conditionally
  detect.
- Python is skipped if no `Python3` interpreter is found. Building
  `PyDomainExtractor` from source additionally requires a Rust
  toolchain (`rustc`/`cargo`) if no pre-built wheel is available for
  your platform/Python version.

Everything else — a Python venv, `pip install`, CMake `FetchContent` —
happens inside that language's own `CMakeLists.txt` once the toolchain
is confirmed present.
