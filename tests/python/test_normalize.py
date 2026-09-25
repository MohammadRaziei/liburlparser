#!/bin/python3
from __future__ import annotations

import pytest

from liburlparser import Url, normalize

# Gap: str() kept the raw path (no dot-segment resolution), kept an
# explicit default port, didn't IDNA-normalize a Unicode host, and the
# parser didn't trim leading/trailing whitespace at all. Expected values
# verified against ada_url.normalize_url() where applicable.

CASES = [
    ("https://example.com/a/../b", "https://example.com/b"),
    ("https://example.com:443/path", "https://example.com/path"),
    ("http://example.com:80/path", "http://example.com/path"),
    ("https://example.com:8443/path", "https://example.com:8443/path"),
    ("https://caf\u00e9.com/path", "https://xn--caf-dma.com/path"),
    ("https://example.com", "https://example.com/"),
    ("https://example.com/path?b=2&a=1#frag", "https://example.com/path?b=2&a=1#frag"),
]


@pytest.mark.parametrize("raw,expected", CASES)
def test_normalized_property(raw, expected):
    assert Url(raw).normalized == expected


@pytest.mark.parametrize("raw,expected", CASES)
def test_normalize_free_function(raw, expected):
    assert normalize(raw) == expected


def test_ssh_keeps_port_no_default_for_non_special_scheme():
    u = Url("ssh://git@example.com:22/repo.git")
    assert u.normalized == "ssh://git@example.com:22/repo.git"


def test_str_unaffected_by_normalized():
    u = Url("https://example.com/a/../b")
    assert str(u) == "https://example.com/a/../b"
    assert u.normalized == "https://example.com/b"


def test_constructor_trims_whitespace():
    assert str(Url("  https://example.com/path  ")) == "https://example.com/path"
    assert str(Url("\t\nhttps://example.com/path\n\t")) == "https://example.com/path"
