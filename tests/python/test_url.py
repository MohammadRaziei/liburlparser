#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest
from liburlparser import IPv4, IPv6, Url, bare_host

with (Path(__file__).parent.parent / "data" / "url_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    url_data_list = list(reader)

@pytest.mark.parametrize("url_data", url_data_list)
def test_url(url_data):
    url = Url(url_data["url"], url_data["ignore_www"] == "True")
    assert url.protocol == url_data["protocol"]
    assert url.userinfo == url_data["userinfo"]
    assert url.host_text == url_data["fulldomain"]
    assert str(url.host) == url_data["fulldomain"]
    host = url.host
    assert host.is_hostname()  # every row in url_data.csv is a domain name
    hostname = host.get_hostname()
    assert hostname.subdomain == url_data["subdomain"]
    assert hostname.domain == url_data["domain"]
    assert hostname.domain_name == url_data["domain_name"]
    assert hostname.suffix == url_data["suffix"]
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


def test_abspath_keeps_the_trailing_slash():
    assert Url("https://example.com/a/").abspath == "/a/"
    assert Url("https://example.com/a").abspath == "/a"
    assert Url("https://example.com/a/b/..").abspath == "/a/"
    assert Url("https://example.com/a/b/.").abspath == "/a/b/"
    assert Url("https://example.com/a/..").abspath == "/"


def test_equality_compares_by_value():
    a = Url("https://example.com/path?x=1")
    b = Url("https://example.com/path?x=1")
    c = Url("https://example.com/other")
    assert a == b
    assert a != c


def test_default_ignore_www_false():
    url = Url("https://www.example.com/")
    assert url.host_text == "www.example.com"


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


# --- Url.host: classified as Hostname, IPv4, or IPv6, wrapped in a Host --
#
# Regression coverage for the hostname/ipv4/ipv6 split: before it, an IPv4
# URL's host was run through Public-Suffix-List matching as if it were a
# domain name, producing nonsense (suffix="1", domain="1" for
# "192.168.1.1"). An IPv6 URL with an explicit port ("[::1]:8080") also
# used to have its own address colons confused with the port separator.

def test_url_host_ipv4_is_classified_not_treated_as_domain():
    url = Url("http://192.168.1.1/path")
    assert url.host.is_ipv4()
    assert isinstance(url.host.get_ipv4(), IPv4)
    assert str(url.host) == "192.168.1.1"


def test_url_host_ipv6_with_port_is_classified_correctly():
    url = Url("http://[2001:db8::1]:8080/x")
    assert url.host.is_ipv6()
    assert isinstance(url.host.get_ipv6(), IPv6)
    assert str(url.host) == "2001:db8::1"


def test_url_host_returns_independent_copy():
    # Regression test: Url.host used to return a live reference into the
    # url's cached host, so mutating the returned IPv4/IPv6 (via
    # get_ipv4()/get_ipv6() and then += 1) silently mutated the url object
    # itself too. Url.host must always be an independent copy.
    url = Url("http://192.168.1.1/path")
    host = url.host
    v4 = host.get_ipv4()
    v4 += 5
    assert str(v4) == "192.168.1.6"
    assert str(url.host) == "192.168.1.1"


@pytest.mark.parametrize(
    ("scheme", "port"),
    [("http", 80), ("https", 443), ("ws", 80), ("wss", 443), ("ftp", 21), ("socks5", 1080), ("socks5h", 1080), ("ssh", 0), ("", 0)],
)
def test_default_port(scheme, port):
    assert Url.default_port(scheme) == port


def test_effective_port():
    assert Url("https://example.com:8443/").effective_port == 8443
    plain = Url("http://example.com/")
    assert plain.port == 0
    assert plain.effective_port == 80
    assert Url("gopher://example.com/").effective_port == 0
    assert Url("socks5://proxy.local").effective_port == 1080
    assert Url("socks5h://proxy.local:9050").effective_port == 9050


@pytest.mark.parametrize(
    ("text", "username", "password"),
    [
        ("https://user:pass@example.com/", "user", "pass"),
        ("https://user:a:b@example.com/", "user", "a:b"),
        ("https://user@example.com/", "user", ""),
        ("https://user:@example.com/", "user", ""),
        ("https://:secret@example.com/", "", "secret"),
        ("https://example.com/", "", ""),
        ("https://us%40er:p%3Aw@example.com/", "us%40er", "p%3Aw"),
    ],
)
def test_username_and_password(text, username, password):
    url = Url(text)
    assert url.username == username
    assert url.password == password


@pytest.mark.parametrize(
    "text, bare",
    [("[::1]", "::1"), ("[2001:db8::1]", "2001:db8::1"), ("::1", "::1"), ("127.0.0.1", "127.0.0.1"), ("example.com", "example.com"), ("", ""), ("[::1", "[::1")],
)
def test_bare_host_on_a_host_held_as_text(text, bare):
    assert bare_host(text) == bare


def test_ipv6_literal_host():
    url = Url("http://[::1]:8080/")
    assert url.host_text == "[::1]"
    assert url.port == 8080
    assert url.host.is_ipv6()
