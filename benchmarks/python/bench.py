#!/usr/bin/env python3
"""liburlparser Python benchmark.

Every implementation here — liburlparser included — is one more peer
entry, installed from its own source (GitHub or PyPI), tested the same
way. No "liburlparser vs X" framing: one shared results table, one row
per (library, operation) pair.

Methodology (mirrors the same shape used by ctoon's own benchmarks):
  1. Load the corpus into memory (untimed).
  2. Timed "extract_from_host": repeatedly extract domain/suffix from a
     bare host string (no scheme, no path) - the exact comparison already
     published in this project's own README.
  3. Timed "extract_from_url": repeatedly extract domain/suffix from a
     full URL (scheme + host + path + query + fragment).
  4. Report throughput (MB/s of bytes actually read by successful
     extractions only) and extractions/sec.

Implementations:
  - liburlparser      this project (github.com/mohammadraziei/liburlparser)
  - tldextract        github.com/john-kurkowski/tldextract
  - PyDomainExtractor  github.com/Intsights/PyDomainExtractor (Rust-backed)
  - tld               github.com/barseghyanartur/tld
  - publicsuffix2     github.com/aboutcode-org/python-publicsuffix2
  - can_ada           github.com/TkTech/can_ada (pybind11 bindings for
                       ada-url's C++ WHATWG URL parser) - "parse_url" only,
                       see the note further down.
"""
import argparse
import sys
import time

import liburlparser

try:
    from tabulate import tabulate
except ImportError:
    tabulate = None

try:
    import tldextract
except ImportError:
    tldextract = None

try:
    import pydomainextractor
except ImportError:
    pydomainextractor = None

try:
    from tld import parse_tld
except ImportError:
    parse_tld = None

try:
    import publicsuffix2
except ImportError:
    publicsuffix2 = None

try:
    import can_ada
except ImportError:
    can_ada = None

REPEATS = 20


def load_corpus(domains_path, urls_path):
    with open(domains_path, "r") as f:
        domains = [line.strip() for line in f if line.strip()]
    with open(urls_path, "r") as f:
        urls = [line.strip() for line in f if line.strip()]
    return domains, urls


def bench(items, fn):
    t0 = time.perf_counter()
    ops = 0
    bytes_done = 0
    for _ in range(REPEATS):
        for item in items:
            try:
                fn(item)
                ops += 1
                bytes_done += len(item.encode("utf-8"))
            except Exception:
                pass
    return time.perf_counter() - t0, ops, bytes_done


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("domains_file")
    parser.add_argument("urls_file")
    parser.add_argument("results_json")
    args = parser.parse_args()

    domains, urls = load_corpus(args.domains_file, args.urls_file)
    if not domains or not urls:
        print("Corpus is empty - nothing to benchmark.", file=sys.stderr)
        return 1

    print("liburlparser Benchmarks — Python")
    print(f"Corpus: {len(domains)} domains, {len(urls)} URLs\n")

    rows = []
    results = []

    def add_rows(name, host_fn, url_fn):
        if host_fn is not None:
            t, ops, nbytes = bench(domains, host_fn)
            rows.append([name, "extract_from_host",
                         f"{nbytes / t / 1e6:.2f} MB/s" if ops else "n/a",
                         f"{ops / t:.0f}", f"{100 * ops / (len(domains) * REPEATS):.0f}%",
                         f"{t:.4f} s"])
            results.append({
                "library": name, "operation": "extract_from_host",
                "throughput_mb_s": (nbytes / t / 1e6) if ops else 0.0,
                "ops_per_sec": ops / t, "success_rate": ops / (len(domains) * REPEATS),
                "total_time_s": t,
            })

        if url_fn is not None:
            t, ops, nbytes = bench(urls, url_fn)
            rows.append([name, "extract_from_url",
                         f"{nbytes / t / 1e6:.2f} MB/s" if ops else "n/a",
                         f"{ops / t:.0f}", f"{100 * ops / (len(urls) * REPEATS):.0f}%",
                         f"{t:.4f} s"])
            results.append({
                "library": name, "operation": "extract_from_url",
                "throughput_mb_s": (nbytes / t / 1e6) if ops else 0.0,
                "ops_per_sec": ops / t, "success_rate": ops / (len(urls) * REPEATS),
                "total_time_s": t,
            })

    # liburlparser: Hostname(host) for a bare host, Hostname.from_url(url)
    # for a full URL - both raise on a malformed input, same as every
    # other library here.
    add_rows(
        "liburlparser",
        lambda host: liburlparser.Hostname(host).suffix,
        lambda url: liburlparser.Hostname.from_url(url).suffix,
    )

    if tldextract:
        # tldextract.extract() accepts either a bare host or a full URL,
        # so the same call covers both operations.
        add_rows("tldextract", tldextract.extract, tldextract.extract)

    if pydomainextractor:
        _pde = pydomainextractor.DomainExtractor()
        add_rows("PyDomainExtractor", _pde.extract, _pde.extract_from_url)

    if parse_tld:
        # tld.parse_tld() expects a scheme; feed bare hosts through
        # fix_protocol so extract_from_host is still a fair "give it a
        # host" comparison rather than an artificial failure.
        add_rows(
            "tld",
            lambda host: parse_tld(host, fix_protocol=True),
            lambda url: parse_tld(url),
        )

    if publicsuffix2:
        # publicsuffix2 has no URL-aware entry point - it expects a bare
        # host, so extract_from_url isn't a like-for-like comparison and
        # is skipped for this library (a real capability gap, not an
        # omission - same treatment ctoon gives a library that can't do
        # one of the two operations).
        add_rows("publicsuffix2", publicsuffix2.get_sld, None)

    # A third operation, "parse_url": split a full URL into its
    # protocol/host/path/query/fragment. can_ada (github.com/TkTech/
    # can_ada, pybind11 bindings for the ada-url C++ engine) has no PSL /
    # domain-extraction feature - same capability gap ada itself has in
    # the C++ benchmark - so it only gets this operation, not
    # extract_from_host/extract_from_url above.
    def add_parse_url_row(name, fn):
        t, ops, nbytes = bench(urls, fn)
        rows.append([name, "parse_url",
                     f"{nbytes / t / 1e6:.2f} MB/s" if ops else "n/a",
                     f"{ops / t:.0f}", f"{100 * ops / (len(urls) * REPEATS):.0f}%",
                     f"{t:.4f} s"])
        results.append({
            "library": name, "operation": "parse_url",
            "throughput_mb_s": (nbytes / t / 1e6) if ops else 0.0,
            "ops_per_sec": ops / t, "success_rate": ops / (len(urls) * REPEATS),
            "total_time_s": t,
        })

    def liburlparser_parse_url(url):
        u = liburlparser.Url(url)
        return u.protocol, u.host_text, u.abspath, u.query, u.fragment

    add_parse_url_row("liburlparser", liburlparser_parse_url)

    if can_ada:
        def can_ada_parse_url(url):
            u = can_ada.parse(url)
            return u.protocol, u.host, u.pathname, u.search, u.hash

        add_parse_url_row("can_ada", can_ada_parse_url)

    headers = ["Library", "Operation", "Throughput", "Ops/sec", "Success", f"Total time (x{REPEATS} reps)"]
    if tabulate:
        print(tabulate(rows, headers=headers))
    else:
        fmt = "{:<18} {:<18} {:>12} {:>10} {:>8} {:>24}"
        print(fmt.format(*headers))
        for row in rows:
            print(fmt.format(*row))

    with open(args.results_json, "w") as f:
        import json
        json.dump({
            "language": "python",
            "corpus": {"domains": len(domains), "urls": len(urls)},
            "results": results,
        }, f, indent=2)
    print(f"\nResults written to {args.results_json}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
