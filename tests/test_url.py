#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest

from liburlparser import Url

with (Path(__file__).parent / "data" / "url_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    url_data_list = list(reader)

@pytest.mark.parametrize("url_data", url_data_list)
def test_url(url_data):
    url = Url(url_data["url"], bool(url_data["ignore_www"]))
    assert url.protocol == url_data["protocol"]
    assert url.userinfo == url_data["userinfo"]
    assert str(url.host) == url_data["host"]
    assert url.domain == url_data["domain"]
    assert url.domain_name == url_data["domain_name"]
    assert url.suffix == url_data["suffix"]
    assert url.port == int(url_data["port"])
    assert url.query == url_data["query"]
    assert url.fragment == url_data["fragment"]


