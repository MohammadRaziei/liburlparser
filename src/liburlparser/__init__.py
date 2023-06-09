from __future__ import annotations
import warnings
import os
import requests

from ._core import __doc__, __version__, Url, Host, load_psl_from_path, load_psl_from_string, is_psl_loaded

def warning_on_one_line(message, category, filename, lineno, file=None, line=None):
    return '%s:%s: %s: %s\n' % (filename, lineno, category.__name__, message)
warnings.formatwarning = warning_on_one_line

def psl_update():
    resp = requests.get("https://publicsuffix.org/list/public_suffix_list.dat")
    load_psl_from_string(resp.text)

if not is_psl_loaded():
    psl_filename = os.path.join(os.path.dirname(__file__), "public_suffix_list.dat")
    if os.path.exists(psl_filename):
        load_psl_from_path(psl_filename)
    else:
        warnings.warn("Cannot find Public_suffix_list.dat. you must import it with \"load_psl_from_path\" or \"load_psl_from_string\" functions", RuntimeWarning, stacklevel=2)

__all__ = [
    "__doc__", "__version__", "Url", "Host", "load_psl_from_file",
    "load_psl_from_string", "is_psl_loaded", "psl_update"
]
