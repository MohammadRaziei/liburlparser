#!/bin/python3
from __future__ import annotations

import pytest

from liburlparser import Scp

# Gap: liburlparser had no support for the "scp-like" SSH address shape
# ([user@]host:path) that scp/rsync/sshfs/git all accept. Rule verified
# against git's own docs: recognized only when there's no "://" and no '/'
# before the first ':'.

SCP_LIKE = [
    "git@github.com:user/repo.git",
    "github.com:user/repo.git",
    "host:22/repo",  # no port disambiguation in this syntax, per git docs
]

NOT_SCP_LIKE = [
    "ssh://git@github.com/user/repo.git",
    "https://github.com/user/repo.git",
    "./foo:bar",
    "/abs/path:something",
    "just/a/path",
    "justahost",
]


@pytest.mark.parametrize("s", SCP_LIKE)
def test_is_scp_like_true(s):
    assert Scp.is_scp_like(s)


@pytest.mark.parametrize("s", NOT_SCP_LIKE)
def test_is_scp_like_false(s):
    assert not Scp.is_scp_like(s)


def test_normalize_basic_case():
    assert Scp.normalize("git@github.com:user/repo.git") == \
        "ssh://git@github.com/user/repo.git"


def test_normalize_leaves_normal_url_unchanged():
    assert Scp.normalize("https://github.com/user/repo.git") == \
        "https://github.com/user/repo.git"


def test_constructor_parses_scp_like_as_ssh_url():
    s = Scp("git@github.com:mohammadraziei/liburlparser.git")
    assert s.was_scp_like
    assert s.url.protocol == "ssh"
    assert s.url.userinfo == "git"
    assert str(s.url.host) == "github.com"
    assert s.url.abspath == "/mohammadraziei/liburlparser.git"


def test_constructor_accepts_normal_url_unchanged():
    s = Scp("https://github.com/mohammadraziei/liburlparser.git")
    assert not s.was_scp_like
    assert s.url.protocol == "https"


def test_repr():
    s = Scp("git@github.com:user/repo.git")
    assert "ssh://git@github.com/user/repo.git" in repr(s)
