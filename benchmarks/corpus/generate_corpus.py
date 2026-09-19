#!/usr/bin/env python3
"""Generates benchmarks/corpus/{domains,urls}.txt.

domains.txt: downloaded verbatim from a public, currently-registered
domain list (Majestic Million-derived) - no processing.

urls.txt: one URL synthesized per domain (a "www." subdomain on every
third entry, a rotating path/query/fragment shape) - but instead of
building each URL as a hand-rolled string (as this used to be, in CMake's
`foreach`/`string(APPEND ...)`), every candidate is round-tripped through
ada_url.URL - the same WHATWG-spec URL parser this project's own
benchmarks compare liburlparser against - and its normalized .href is
what actually gets written. Anything that fails to parse gets dropped
(reported, not silently skipped), instead of trusting the raw
concatenation was well-formed.

Invoked from benchmarks/CMakeLists.txt at configure time; safe to run
directly too (skips work for files that already exist).
"""
import sys
import urllib.request
from pathlib import Path

CORPUS_DIR = Path(__file__).resolve().parent
DOMAINS_FILE = CORPUS_DIR / "domains.txt"
URLS_FILE = CORPUS_DIR / "urls.txt"
DOMAINS_SOURCE = "https://raw.githubusercontent.com/felmoltor/robotstxt/master/top.10000.majestic.domains.txt"


def ensure_domains() -> list[str]:
    if not DOMAINS_FILE.exists():
        print(f"Downloading domain corpus from {DOMAINS_SOURCE} ...", file=sys.stderr)
        with urllib.request.urlopen(DOMAINS_SOURCE, timeout=30) as resp:
            DOMAINS_FILE.write_bytes(resp.read())
    domains = [line.strip() for line in DOMAINS_FILE.read_text().splitlines() if line.strip()]
    print(f"domains.txt: {len(domains)} domains", file=sys.stderr)
    return domains


def ensure_urls(domains: list[str]) -> None:
    if URLS_FILE.exists():
        return

    import ada_url  # deferred: only needed for this step

    paths = [
        "/",
        "/index.html?ref=bench&id={i}",
        "/articles/{i}/view",
        "/search?q=test+{i}#results",
    ]

    urls: list[str] = []
    dropped = 0
    for i, domain in enumerate(domains):
        host = f"www.{domain}" if i % 3 == 0 else domain
        candidate = f"{'https' if i % 4 != 2 else 'http'}://{host}{paths[i % 4].format(i=i)}"
        try:
            # Round-trips through ada_url's WHATWG-spec parser/normalizer
            # instead of trusting the hand-built string directly - anything
            # that doesn't parse cleanly gets dropped, not shipped.
            urls.append(ada_url.URL(candidate).href)
        except Exception:
            dropped += 1

    URLS_FILE.write_text("\n".join(urls) + "\n")
    print(f"urls.txt: {len(urls)} urls written, {dropped} candidates dropped (failed ada_url parse)", file=sys.stderr)


if __name__ == "__main__":
    ensure_urls(ensure_domains())
