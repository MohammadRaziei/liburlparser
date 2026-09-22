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
  4. Timed "parse_url": split a full URL into protocol/host/path/query/
     fragment.
  5. Timed "idna_normalize": normalize a Unicode hostname to its
     canonical ASCII/Punycode form (liburlparser.Hostname.normalized_ascii,
     new - see include/idna.h).
  6. Timed "percent_decode"/"percent_encode": liburlparser.unquote/quote
     (new, from-scratch, no dependency - see include/percent_codec.h)
     against Python's own urllib.parse.unquote/quote.
  7. Report throughput (MB/s of bytes actually read by successful
     extractions only) and extractions/sec.

Implementations:
  - liburlparser      this project (github.com/mohammadraziei/liburlparser)
  - tldextract        github.com/john-kurkowski/tldextract
  - PyDomainExtractor  github.com/Intsights/PyDomainExtractor (Rust-backed)
  - tld               github.com/barseghyanartur/tld
  - publicsuffix2     github.com/aboutcode-org/python-publicsuffix2
  - can_ada           github.com/TkTech/can_ada (pybind11 bindings for
                       ada-url's C++ WHATWG URL parser) - "parse_url" and
                       "idna_normalize" only, see the notes further down.
  - urllib.parse      Python stdlib - "percent_decode"/"percent_encode"
                       only (it has no URL-parsing or IDNA entry point
                       comparable to the others here).
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

import urllib.parse as urllib_parse

REPEATS = 20
WARMUP_REPS = 3


def load_corpus(domains_path, urls_path):
    with open(domains_path, "r") as f:
        domains = [line.strip() for line in f if line.strip()]
    with open(urls_path, "r") as f:
        urls = [line.strip() for line in f if line.strip()]
    return domains, urls


def bench(items, fn):
    # Untimed warm-up: WARMUP_REPS full passes over the corpus with the
    # exact same call, before the timer starts. Every library gets this,
    # not just liburlparser - any of them can have first-call costs (lazy
    # regex compilation, parsing a bundled suffix-list snapshot, building
    # an internal trie, module-level caches) that would otherwise get
    # counted inside the timed measurement below.
    for _ in range(WARMUP_REPS):
        for item in items:
            try:
                fn(item)
            except Exception:
                pass

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

    # liburlparser: Hostname.extract_dict_from_host/_url() are single-FFI-
    # call, dict-returning static methods (see src/bindings/python/binding_py.cpp) - the
    # apples-to-apples match for PyDomainExtractor.extract()'s shape
    # below, rather than Hostname(host).suffix which does a separate
    # object-construction call and only returns one field.
    add_rows(
        "liburlparser",
        liburlparser.Hostname.extract_dict_from_host,
        liburlparser.Hostname.extract_dict_from_url,
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

    # A fourth operation, "idna_normalize": normalize a Unicode hostname to
    # its canonical ASCII/Punycode form. liburlparser's
    # Hostname.normalized_ascii is new (see include/idna.h) - it closed a
    # real gap where liburlparser accepted Unicode hosts without error but
    # never normalized them, so "café.com" and "xn--caf-dma.com" used to
    # compare as different hostnames. can_ada.idna_encode is the peer here
    # for the same reason it is can_ada's peer above: pybind11 bindings
    # over the same ada-url C++ engine, not the cffi-based ada_url package.
    IDNA_HOSTS = [
        "café.com", "münchen.de", "españa.es", "português.pt",
        "中国.icom.museum", "日本.jp", "россия.рф", "ελλάδα.gr",
        "www.7\u2011Eleven.com", "دامنه.ایران",
        "example.com", "xn--caf-dma.com",  # ASCII / already-punycoded controls
    ]

    def add_idna_row(name, fn):
        t, ops, nbytes = bench(IDNA_HOSTS, fn)
        rows.append([name, "idna_normalize",
                     f"{nbytes / t / 1e6:.2f} MB/s" if ops else "n/a",
                     f"{ops / t:.0f}", f"{100 * ops / (len(IDNA_HOSTS) * REPEATS):.0f}%",
                     f"{t:.4f} s"])
        results.append({
            "library": name, "operation": "idna_normalize",
            "throughput_mb_s": (nbytes / t / 1e6) if ops else 0.0,
            "ops_per_sec": ops / t, "success_rate": ops / (len(IDNA_HOSTS) * REPEATS),
            "total_time_s": t,
        })

    add_idna_row(
        "liburlparser",
        lambda h: liburlparser.Hostname(h).normalized_ascii,
    )
    if can_ada:
        add_idna_row("can_ada", can_ada.idna_encode)

    # A fifth/sixth operation, "percent_decode"/"percent_encode":
    # liburlparser.unquote/quote are new (see include/percent_codec.h) -
    # a from-scratch, dependency-free implementation (explicitly *not*
    # linked against ada, unlike idna_normalize above). The only
    # comparable peer here is Python's own urllib.parse, since that is
    # what people reach for today in liburlparser's absence; can_ada and
    # ada-url's C++ core don't expose a standalone percent-encode/decode
    # entry point the way they do parse_url/idna.
    PERCENT_ENCODED = [
        "hello%20world", "caf%C3%A9%20%26%20friends",
        "q%3Dhello%20world%26more%3Dstuff", "100%25%20done",
        "a%2Fb%2Fc%2Fd", "%E6%97%A5%E6%9C%AC%E8%AA%9E",
    ]
    PERCENT_RAW = [
        "hello world", "café & friends", "q=hello world&more=stuff",
        "100% done", "a/b/c/d", "日本語",
    ]

    def add_percent_row(name, op, corpus, fn):
        t, ops, nbytes = bench(corpus, fn)
        rows.append([name, op,
                     f"{nbytes / t / 1e6:.2f} MB/s" if ops else "n/a",
                     f"{ops / t:.0f}", f"{100 * ops / (len(corpus) * REPEATS):.0f}%",
                     f"{t:.4f} s"])
        results.append({
            "library": name, "operation": op,
            "throughput_mb_s": (nbytes / t / 1e6) if ops else 0.0,
            "ops_per_sec": ops / t, "success_rate": ops / (len(corpus) * REPEATS),
            "total_time_s": t,
        })

    add_percent_row("liburlparser", "percent_decode", PERCENT_ENCODED, liburlparser.unquote)
    add_percent_row("urllib.parse", "percent_decode", PERCENT_ENCODED, urllib_parse.unquote)
    add_percent_row("liburlparser", "percent_encode", PERCENT_RAW, liburlparser.quote)
    add_percent_row("urllib.parse", "percent_encode", PERCENT_RAW, urllib_parse.quote)

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
