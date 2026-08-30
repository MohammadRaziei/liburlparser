#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest

from liburlparser import Url

with (Path(__file__).parent.parent / "data" / "url_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    url_data_list = list(reader)

@pytest.mark.parametrize("url_data", url_data_list)
def test_url(url_data):
    url = Url(url_data["url"], url_data["ignore_www"] == "True")
    assert url.protocol == url_data["protocol"]
    assert url.userinfo == url_data["userinfo"]
    assert url.full_domain == url_data["fulldomain"]
    assert str(url.host) == url_data["fulldomain"]
    assert url.subdomain == url_data["subdomain"]
    assert url.domain == url_data["domain"]
    assert url.domain_name == url_data["domain_name"]
    assert url.suffix == url_data["suffix"]
    assert url.port == int(url_data["port"])
    assert url.query == url_data["query"]
    assert url.fragment == url_data["fragment"]


def test_str_round_trips():
    url = Url("https://example.com/path?a=1#frag")
    assert str(url) == "https://example.com/path?a=1#frag"


def test_params_splits_on_ampersand():
    url = Url("https://example.com/?a=1&b=2")
    assert url.params == ["a=1", "b=2"]


def test_abspath_resolves_dot_dot():
    url = Url("https://example.com/a/b/../c")
    assert url.abspath == "/a/c"


def test_equality_compares_by_value():
    a = Url("https://example.com/path?x=1")
    b = Url("https://example.com/path?x=1")
    c = Url("https://example.com/other")
    assert a == b
    assert a != c


def test_default_ignore_www_false():
    url = Url("https://www.example.com/")
    assert url.full_domain == "www.example.com"


# --- Url.extract_host(): static method exposed via nanobind's def_static.
# Regression coverage for two bugs found while adding a move-semantics
# overload to the C++ side: (1) extract_host used to leave a port number
# glued onto the host, and a lone '#' fragment with no path/query wasn't
# recognized as ending the host either; (2) the fix's new C++ overloads
# made `&urlparser::url::extract_host` ambiguous for nanobind's def_static,
# which broke the Python module's build entirely. If either regresses, this
# test (or just importing `liburlparser` at all) will fail.

def test_extract_host_strips_port():
    assert Url.extract_host("https://www.example.com:8080/path") == "www.example.com"


def test_extract_host_stops_at_fragment_with_no_path():
    assert Url.extract_host("https://example.com#frag") == "example.com"


def test_extract_host_does_not_confuse_port_with_userinfo_colon():
    assert Url.extract_host("https://user:pass@host.example.com:80/x") == "host.example.com"


