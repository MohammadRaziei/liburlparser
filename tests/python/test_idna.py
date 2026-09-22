#!/bin/python3
from __future__ import annotations

import pytest

from liburlparser import Hostname

# Same gap identified by comparing against ada-url/ada: liburlparser accepts
# non-ASCII (Unicode) hosts without error, but keeps them verbatim instead of
# normalizing to the canonical ASCII/Punycode form - so "café.com" and
# "xn--caf-dma.com" used to compare as different hostnames even though they
# are the same domain. `Hostname.normalized_ascii` (C++: idna::to_ascii(),
# delegating to ada::idna::to_ascii()) closes that gap.

IDNA_CASES = [
    ("https://example.com", "example.com"),
    ("https://xn--7eleven-506c.com", "xn--7eleven-506c.com"),
    ("https://café.com", "xn--caf-dma.com"),
    ("https://münchen.de", "xn--mnchen-3ya.de"),
    ("https://www.caf\u00e9-shop.com", "www.xn--caf-shop-d1a.com"),
    ("https://دامنه.ایران", "xn--mgbp1eef.xn--mgba3a4f16a"),
]


@pytest.mark.parametrize("url,expected_ascii", IDNA_CASES)
def test_normalized_ascii(url, expected_ascii):
    host = Hostname.from_url(url)
    assert host.normalized_ascii == expected_ascii


def test_str_stays_unicode_while_normalized_ascii_is_punycode():
    host = Hostname.from_url("https://café.com")
    assert str(host) == "café.com"
    assert host.normalized_ascii == "xn--caf-dma.com"


def test_unicode_and_punycode_hosts_now_normalize_equal():
    unicode_host = Hostname.from_url("https://café.com")
    ascii_host = Hostname.from_url("https://xn--caf-dma.com")
    # str()/__eq__ still differ (deliberately - see idna.h), but the
    # canonical form now agrees, which is the whole point of the feature.
    assert unicode_host != ascii_host
    assert unicode_host.normalized_ascii == ascii_host.normalized_ascii
