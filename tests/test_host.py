#!/bin/python3
from __future__ import annotations

import csv
from pathlib import Path

import pytest

from liburlparser import Host

with (Path(__file__).parent / "data" / "host_data.csv").open("r") as f:
    reader = csv.DictReader(f)
    host_data_list = list(reader)

@pytest.mark.parametrize("host_data", host_data_list)
def test_host(host_data):
    host = Host.from_url(host_data["url"], bool(host_data["ignore_www"]))
    assert str(host) == host_data["host"]
    assert host.domain == host_data["domain"]
    assert host.domain_name == host_data["domain_name"]
    assert host.suffix == host_data["suffix"]
