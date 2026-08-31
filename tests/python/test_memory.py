#!/bin/python3
"""Memory-leak regression tests for the liburlparser nanobind extension.

Run explicitly with memray enabled (it's disabled by default so normal
`pytest` runs stay fast):

    pytest tests/python/test_memory.py --memray

`@pytest.mark.limit_leaks` fails the test if the process's net allocations
after the marked function returns exceed the given threshold - i.e. if
objects created inside are not being freed. This targets the C++/Python
boundary (nanobind wrapper objects, and - since url::host()/parse_host()
build a std::variant<hostname, ipv4, ipv6> - nanobind's variant caster
dispatching to whichever alternative's own caster applies), which is
exactly where reference-counting bugs would show up.
"""
from __future__ import annotations

import gc

import pytest

from liburlparser import Hostname, Url, parse_host_from_url

URLS = [
    "https://www.example.com/path?query=1",
    "http://sub.example.co.uk/",
    "https://api.github.io/v1/resource",
    "https://blog.subdomain.example.com.au/post/123",
    "http://plain.org",
    "http://192.168.1.1:8080/admin",
    "http://[2001:db8::1]:8080/x",
]


@pytest.mark.limit_leaks("1 MB")
def test_url_construction_does_not_leak():
    for _ in range(20_000):
        for u in URLS:
            Url(u)
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_hostname_construction_does_not_leak():
    for _ in range(20_000):
        for u in URLS:
            Hostname.from_url(u)
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_hostname_lazy_fields_do_not_leak():
    """Exercises the lazy-parse path (ensure_parsed) repeatedly, since that's
    the PSL-dependent code path (caches domain/subdomain/suffix on first access)."""
    for _ in range(20_000):
        for u in URLS:
            h = Hostname.from_url(u)
            _ = h.domain, h.subdomain, h.suffix, h.full_domain


@pytest.mark.limit_leaks("1 MB")
def test_url_host_roundtrip_does_not_leak():
    """Url -> .host -> str, the most common real-world call chain - and, since
    URLS includes IPv4/IPv6 hosts too, exercises all three variant
    alternatives' casters (Hostname/IPv4/IPv6), not just the hostname one."""
    for _ in range(20_000):
        for u in URLS:
            url = Url(u)
            _ = str(url.host)
    gc.collect()


@pytest.mark.limit_leaks("1 MB")
def test_parse_host_from_url_does_not_leak():
    """Exercises parse_host_from_url()'s std::variant<hostname, ipv4, ipv6>
    return value directly (not via Url.host), across all three URLS' host
    kinds, plus the to_dict() dispatch on whichever type comes back."""
    for _ in range(20_000):
        for u in URLS:
            host = parse_host_from_url(u)
            _ = host.to_dict()
