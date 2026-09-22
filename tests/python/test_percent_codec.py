#!/bin/python3
from __future__ import annotations

import pytest

from liburlparser import quote, unquote

# Gap: liburlparser's url.query()/params()/abspath() return the raw,
# percent-encoded substrings by design (matching WHATWG/ada), but exposed no
# way to decode/encode them yourself - unlike Python's own
# urllib.parse.quote/unquote. This adds that missing pair, implemented from
# scratch in C++ (no dependency), just exposed to Python.

UNQUOTE_CASES = [
    ("hello", "hello"),
    ("hello%20world", "hello world"),
    ("caf%C3%A9", "café"),
    ("caf%c3%a9", "café"),
    ("hello%20world%26more", "hello world&more"),
    ("100%", "100%"),
    ("100%zz", "100%zz"),
    ("", ""),
]


@pytest.mark.parametrize("encoded,expected", UNQUOTE_CASES)
def test_unquote(encoded, expected):
    assert unquote(encoded) == expected


QUOTE_CASES = [
    ("abcXYZ019-_.~", "abcXYZ019-_.~"),
    ("hello world", "hello%20world"),
    ("café", "caf%C3%A9"),
    ("a/b&c", "a%2Fb%26c"),
    ("", ""),
]


@pytest.mark.parametrize("raw,expected", QUOTE_CASES)
def test_quote_default_safe_set(raw, expected):
    assert quote(raw) == expected


def test_quote_keep_slash():
    assert quote("a/b c", keep_slash=True) == "a/b%20c"


def test_roundtrip():
    original = "café & friends / co."
    assert unquote(quote(original)) == original


def test_matches_liburlparser_raw_query_field():
    # End-to-end sanity check against liburlparser's own (raw) query field.
    from liburlparser import Url

    url = Url("https://example.com/search?name=caf%C3%A9&q=a%20b")
    assert url.query == "name=caf%C3%A9&q=a%20b"
    assert unquote(url.query) == "name=café&q=a b"
