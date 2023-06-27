from __future__ import annotations

from ._core import __doc__, __version__, Url, Host, load_psl_from_path, load_psl_from_string, is_psl_loaded

if not is_psl_loaded():
    import os
    load_psl_from_path(os.path.join(os.path.dirname(__file__), "public_suffix_list.dat"))

__all__ = [
    "__doc__", "__version__", "Url", "Host", "load_psl_from_file",
    "load_psl_from_string", "is_psl_loaded"
]
