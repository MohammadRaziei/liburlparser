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


# --- search_params: decoded key -> values dict ---------------------------
# Gap: Url.params only ever gave the raw, still percent-encoded
# "key=value" strings; there was no built-in way to get "the value of
# query parameter x" without manually splitting and unquoting yourself.
# Shape matches ada_url.parse_search_params().

def test_search_params_simple():
    u = Url("https://example.com/?a=1&b=2")
    assert u.search_params == {"a": ["1"], "b": ["2"]}


def test_search_params_repeated_keys_collect_in_order():
    u = Url("https://example.com/?a=1&a=3&b=2")
    assert u.search_params == {"a": ["1", "3"], "b": ["2"]}


def test_search_params_percent_decodes_keys_and_values():
    u = Url("https://example.com/?name=caf%C3%A9&q=a%20b")
    assert u.search_params == {"name": ["café"], "q": ["a b"]}


def test_search_params_valueless_key_gets_empty_string():
    u = Url("https://example.com/?flag&a=1")
    assert u.search_params == {"flag": [""], "a": ["1"]}


def test_search_params_empty_when_no_query():
    assert Url("https://example.com/").search_params == {}


def test_params_stays_raw_and_unchanged():
    # No breaking change: params is still the raw list.
    u = Url("https://example.com/?a=1&b=caf%C3%A9")
    assert u.params == ["a=1", "b=caf%C3%A9"]


def test_search_params_plus_is_space():
    u = Url("https://example.com/?q=hello+world&a+b=1")
    assert u.search_params["q"] == ["hello world"]
    assert u.search_params["a b"] == ["1"]


def test_search_params_encoded_plus_stays_a_plus():
    u = Url("https://example.com/?a=%2B")
    assert u.search_params["a"] == ["+"]


def test_search_params_invalid_utf8_becomes_replacement_char_not_an_exception():
    # A single legacy-encoded parameter ("caf%E9", Latin-1) must not make
    # the whole property raise - the well-formed parameters next to it
    # (here, "ok") still come through fine.
    u = Url("https://example.com/?name=caf%E9&ok=1")
    assert u.search_params["ok"] == ["1"]
    assert u.search_params["name"] == ["caf\ufffd"]
