from __future__ import annotations

import os as _os
import sys as _sys

_pkg_dir = _os.path.dirname(__file__)

# The wheel ships the C++ core as a shared library in lib/ next to this file. Linux
# and macOS find it through the extension's rpath; Windows has no rpath, so the
# directory is added to the DLL search path before the extension is imported.
if _sys.platform == "win32" and hasattr(_os, "add_dll_directory"):
    _lib_dir = _os.path.join(_pkg_dir, "lib")
    if _os.path.isdir(_lib_dir):
        _os.add_dll_directory(_lib_dir)

from .core import Host, Hostname, IPv4, IPv6, ScpUrl, GitUrl, Url, __doc__, __version__, psl, quote, unquote, resolve, normalize, build_search_params  # noqa: E402


def get_include_dir() -> str:
    """Directory containing urlparser.h, for a plain compiler ``-I`` flag."""
    return _os.path.join(_pkg_dir, "include")


def get_lib_dir() -> str:
    """Directory containing the compiled C++ library (the shared liburlparser, plus its
    import library on Windows)."""
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
