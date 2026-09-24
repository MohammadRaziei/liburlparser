#!/bin/python3
from __future__ import annotations

import pytest

from liburlparser import resolve

# Gap: liburlparser had no URL reference-resolution ("join") at all before
# this. Expected values verified against ada_url.join_url() (WHATWG).

CASES = [
    ("https://example.com/a/b/c", "../d", "https://example.com/a/d"),
    ("https://example.com/a/b/c", "/absolute", "https://example.com/absolute"),
    ("https://example.com/a/b/c?x=1", "e?y=2", "https://example.com/a/b/e?y=2"),
    ("https://example.com/a/b/", "./e/f", "https://example.com/a/b/e/f"),
    ("https://example.com/a/b/c", "https://other.com/z", "https://other.com/z"),
    ("https://example.com/a/b/c", "//other.com/z", "https://other.com/z"),
    ("https://example.com/a/b/c", "..", "https://example.com/a/"),
    ("https://example.com/a/b/c", "../../..", "https://example.com/"),
    ("https://example.com/a/b/c", "", "https://example.com/a/b/c"),
    ("https://example.com/a/b/c", "?q=1", "https://example.com/a/b/c?q=1"),
    ("https://example.com:8080/a/b", "../c", "https://example.com:8080/c"),
    ("https://example.com/a/b/", "", "https://example.com/a/b/"),
]


@pytest.mark.parametrize("base,ref,expected", CASES)
def test_resolve(base, ref, expected):
    assert resolve(base, ref) == expected


def test_fragment_dropped_when_ref_has_none_even_if_base_had_one():
    # RFC 3986 §5.2.2: T.fragment = R.fragment, always - not a fallback to
    # Base.fragment. Matches WHATWG/ada_url here.
    assert resolve("https://example.com/a/b/c#f", "..") == "https://example.com/a/"


def test_invalid_base_returns_empty_string():
    assert resolve("not a url", "x") == ""
