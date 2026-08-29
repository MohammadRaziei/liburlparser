#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest

from liburlparser import Host

with (Path(__file__).parent.parent / "data" / "host_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    host_data_list = list(reader)

@pytest.mark.parametrize("host_data", host_data_list)
def test_host(host_data):
    host = Host.from_url(host_data["url"], host_data["ignore_www"] == "True")
    assert str(host) == host_data["host"]
    assert host.subdomain == host_data["subdomain"]
    assert host.domain == host_data["domain"]
    assert host.domain_name == host_data["domain_name"]
    assert host.suffix == host_data["suffix"]


def test_equality_compares_by_value():
    a = Host("example.com")
    b = Host("example.com")
    c = Host("other.com")
    assert a == b
    assert a != c


def test_equality_against_string():
    host = Host("example.com")
    assert host == "example.com"
    assert host != "other.com"


def test_fulldomain_ignore_www():
    assert Host("www.example.com", True).full_domain == "example.com"
    assert Host("www.example.com", False).full_domain == "www.example.com"
