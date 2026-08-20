#!/bin/python3
from __future__ import annotations

import pytest
from urllib.error import URLError

from liburlparser import psl


@pytest.fixture
def disable_urlopen(monkeypatch):
    def request_get(*args, **kwargs):
        msg = "Fake Connection Error :)"
        raise URLError(msg)
    monkeypatch.setattr("liburlparser.core.urlopen", request_get)

def test_psl_update(disable_urlopen):
    with pytest.raises(URLError):
        psl.update()

def test_psl_loaded():
    assert psl.is_loaded()
