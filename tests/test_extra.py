#!/bin/python3
from __future__ import annotations

import pytest
import requests

from liburlparser import psl


@pytest.fixture()
def disable_requests_get(monkeypatch):
    def request_get(*args, **kwargs):
        msg = "Fake Connection Error :)"
        raise requests.exceptions.ConnectionError(msg)
    monkeypatch.setattr(requests, "get", request_get)

def test_psl_update(disable_requests_get):
    with pytest.raises(requests.exceptions.ConnectionError):
        psl.update()

def test_psl_loaded():
    assert psl.is_loaded()
