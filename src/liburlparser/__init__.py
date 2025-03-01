from __future__ import annotations

from ._about import __version__
from .core import Host, Url, __doc__, psl

__all__ = [
    "Host",
    "Url",
    "__doc__",
    "__version__",
    "psl"
]
