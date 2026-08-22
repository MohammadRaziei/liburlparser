#!/bin/python3
"""Memory-leak regression tests for the liburlparser nanobind extension.

Run explicitly with memray enabled (it's disabled by default so normal
`pytest` runs stay fast):

    pytest tests/python/test_memory.py --memray

`@pytest.mark.limit_leaks` fails the test if the process's net allocations
after the marked function returns exceed the given threshold - i.e. if
objects created inside are not being freed. This targets the C++/Python
boundary (nanobind wrapper objects + the PIMPL `shared_ptr<Impl>` on the
C++ side), which is exactly where reference-counting bugs would show up.
"""
from __future__ import annotations

import gc

import pytest

from liburlparser import Host, Url

URLS = [
    "https://www.example.com/path?query=1",
    "http://sub.example.co.uk/",
    "https://api.github.io/v1/resource",
    "https://blog.subdomain.example.com.au/post/123",
    "http://plain.org",
]


@pytest.mark.limit_leaks("1 MB")
def test_url_construction_does_not_leak():
    for _ in range(20_000):
        for u in URLS:
            Url(u)
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_host_construction_does_not_leak():
    for _ in range(20_000):
        for u in URLS:
            Host.from_url(u)
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_host_lazy_fields_do_not_leak():
    """Exercises the lazy-parse path (ensureParsed) repeatedly, since that's
    the newest code path (caches domain/subdomain/suffix on first access)."""
    for _ in range(20_000):
        for u in URLS:
            h = Host.from_url(u)
            _ = h.domain, h.subdomain, h.suffix, h.fulldomain
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_url_host_roundtrip_does_not_leak():
    """Url -> .host -> field access, the most common real-world call chain."""
    for _ in range(20_000):
        for u in URLS:
            url = Url(u)
            _ = url.host.fulldomain
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_host_extract_from_url_does_not_leak():
    """Exercises Host::fromUrl + the dict-conversion path (a separate
    nanobind boundary crossing: C++ std::string -> Python dict)."""
    for _ in range(20_000):
        for u in URLS:
            Host.extract_from_url(u)
