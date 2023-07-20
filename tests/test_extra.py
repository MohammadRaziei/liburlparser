from __future__ import annotations

import pytest
import requests

import liburlparser


@pytest.fixture()
def disable_requests_get(monkeypatch):
    def request_get(*args, **kwargs):
        msg = "Fake Connection Error :)"
        raise requests.exceptions.ConnectionError(msg)
    monkeypatch.setattr(requests, "get", request_get)

def test_psl_update(disable_requests_get):
    with pytest.raises(requests.exceptions.ConnectionError):
        liburlparser.psl.update()
