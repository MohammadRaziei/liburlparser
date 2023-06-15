from __future__ import annotations

from ._core import __doc__, __version__, Url, Host, load_psl_from_path, load_psl_from_string, is_psl_loaded

if not is_psl_loaded():
    import warnings
    warnings.warn("PSL is not loaded! it may cause some perfemormace issues and return unexpected results!")


__all__ = [
    "__doc__", "__version__", "Url", "Host", "load_psl_from_file",
    "load_psl_from_string", "is_psl_loaded"
]
