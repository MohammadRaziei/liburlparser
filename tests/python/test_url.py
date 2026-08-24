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
    assert url.fulldomain == url_data["fulldomain"]
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
    assert url.fulldomain == "www.example.com"


