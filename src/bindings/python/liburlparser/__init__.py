from __future__ import annotations

import os as _os

from .core import Host, Hostname, IPv4, IPv6, ScpUrl, GitUrl, Url, __doc__, __version__, psl, quote, unquote, resolve, normalize, build_search_params

_pkg_dir = _os.path.dirname(__file__)


def get_include_dir() -> str:
    """Directory containing urlparser.h, for a plain compiler ``-I`` flag."""
    return _os.path.join(_pkg_dir, "include")


def get_lib_dir() -> str:
    """Directory containing the compiled static C++ library (liburlparser)."""
    return _os.path.join(_pkg_dir, "lib")


def get_cmake_dir() -> str:
    """Directory containing liburlparserConfig.cmake, for
    ``find_package(liburlparser CONFIG)``."""
    return _os.path.join(_pkg_dir, "cmake")


__all__ = [
    "Host",
    "Hostname",
    "IPv4",
    "IPv6",
    "ScpUrl",
    "GitUrl",
    "Url",
    "__doc__",
    "__version__",
    "psl",
    "quote",
    "unquote",
    "resolve",
    "normalize",
    "build_search_params",
    "get_include_dir",
    "get_lib_dir",
    "get_cmake_dir",
]
