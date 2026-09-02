#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest

from liburlparser import Host, Hostname, IPv4, IPv6

with (Path(__file__).parent.parent / "data" / "host_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    host_data_list = list(reader)

@pytest.mark.parametrize("host_data", host_data_list)
def test_host(host_data):
    host = Hostname.from_url(host_data["url"], host_data["ignore_www"] == "True")
    assert str(host) == host_data["host"]
    assert host.subdomain == host_data["subdomain"]
    assert host.domain == host_data["domain"]
    assert host.domain_name == host_data["domain_name"]
    assert host.suffix == host_data["suffix"]


def test_equality_compares_by_value():
    a = Hostname("example.com")
    b = Hostname("example.com")
    c = Hostname("other.com")
    assert a == b
    assert a != c


def test_equality_against_string():
    host = Hostname("example.com")
    assert host == "example.com"
    assert host != "other.com"


def test_fulldomain_ignore_www():
    assert Hostname("www.example.com", True).full_domain == "example.com"
    assert Hostname("www.example.com", False).full_domain == "www.example.com"


# --- IPv4 --------------------------------------------------------------

def test_ipv4_parse_and_str_roundtrip():
    a = IPv4("192.168.1.1")
    assert str(a) == "192.168.1.1"


def test_ipv4_int_conversion():
    a = IPv4("192.168.1.1")
    assert int(a) == 0xC0A80101
    b = IPv4.from_int(0x08080808)
    assert str(b) == "8.8.8.8"


def test_ipv4_is_valid():
    assert IPv4.is_valid("1.2.3.4")
    assert not IPv4.is_valid("www.example.com")
    assert not IPv4.is_valid("999.1.1.1")


def test_ipv4_parse_raises_on_garbage():
    with pytest.raises(Exception):
        IPv4("not-an-ip")


def test_ipv4_arithmetic_and_wraparound():
    a = IPv4("255.255.255.255")
    assert str(a + 1) == "0.0.0.0"
    assert str(a - 0) == "255.255.255.255"

    b = IPv4("10.0.0.0")
    b += 256
    assert str(b) == "10.0.1.0"


def test_ipv4_comparisons_and_hash():
    a = IPv4("1.0.0.0")
    b = IPv4("2.0.0.0")
    assert a < b
    assert a == IPv4("1.0.0.0")
    assert hash(a) == hash(IPv4("1.0.0.0"))
    assert len({a, b, IPv4("1.0.0.0")}) == 2


def test_ipv4_distance():
    a = IPv4("1.2.3.4")
    b = IPv4("1.2.3.5")
    assert b - a == 1


# --- IPv6 --------------------------------------------------------------

def test_ipv6_parse_bracketed_and_bare_agree():
    a = IPv6("[2001:db8::1]")
    b = IPv6("2001:db8::1")
    assert a == b
    assert str(a) == "2001:db8::1"


def test_ipv6_is_valid():
    assert IPv6.is_valid("::1")
    assert IPv6.is_valid("[::1]")
    assert not IPv6.is_valid("192.168.1.1")


def test_ipv6_high_low_and_arithmetic():
    a = IPv6.from_uint64_pair(0, 0xFFFFFFFFFFFFFFFF)
    a += 1
    assert a.high64 == 1
    assert a.low64 == 0


# --- Host: hostname/ipv4/ipv6 classification -----------------------------

def test_host_classifies_correctly():
    assert Host("example.com").is_hostname()
    assert Host("192.0.2.1").is_ipv4()
    assert Host("[::1]").is_ipv6()


def test_host_from_url_extracts_and_classifies():
    host = Host.from_url("http://192.168.1.1:8080/x")
    assert host.is_ipv4()
    assert str(host) == "192.168.1.1"

    host2 = Host.from_url("https://www.example.com/x", True)
    assert host2.is_hostname()
    assert str(host2) == "example.com"


def test_host_from_url_ipv6_with_port():
    host = Host.from_url("http://[2001:db8::1]:8080/x")
    assert host.is_ipv6()
    assert str(host) == "2001:db8::1"


def test_host_get_and_try_accessors():
    host = Host("192.168.1.1")
    assert host.get_ipv4() == IPv4("192.168.1.1")
    assert host.try_hostname() is None
    assert host.try_ipv4() is not None
    with pytest.raises(Exception):
        host.get_hostname()

