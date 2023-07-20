import pytest
import liburlparser
import requests

@pytest.fixture
def disable_requests_get(monkeypatch):
    def request_get(*args, **kwargs):
        raise requests.exceptions.ConnectionError("Fake Connection Error :)")
    monkeypatch.setattr(requests, "get", request_get)

def test_psl_update(disable_requests_get):
    with pytest.raises(requests.exceptions.ConnectionError):
        liburlparser.psl.update()
