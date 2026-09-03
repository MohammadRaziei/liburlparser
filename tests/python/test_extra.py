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


def test_psl_is_suffix_recognizes_real_entries():
    assert psl.is_suffix("com")
    assert psl.is_suffix("co.uk")
    assert psl.is_suffix("com.au")
    assert psl.is_suffix("github.io")


def test_psl_is_suffix_rejects_non_suffixes():
    assert not psl.is_suffix("comm")           # made-up word, not a real TLD
    assert not psl.is_suffix("example.com")    # a full domain, not a suffix itself
    assert not psl.is_suffix("")


def test_psl_is_suffix_case_insensitive():
    assert psl.is_suffix("COM")
    assert psl.is_suffix("Co.Uk")
