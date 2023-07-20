from __future__ import annotations
import warnings
import os
import requests

from ._core import __doc__, __version__, Url, Host, Psl
psl = Psl() # psl

def warning_on_one_line(message, category, filename, lineno, file=None, line=None):
    return '%s:%s: %s: %s\n' % (filename, lineno, category.__name__, message)
warnings.formatwarning = warning_on_one_line


def psl_update():
    resp = requests.get(psl.url())
    psl.load_from_string(resp.text)

psl.update = psl_update

if not psl.is_loaded():
    psl_filename = os.path.join(os.path.dirname(__file__), "public_suffix_list.dat")
    if os.path.exists(psl_filename):
        psl.load_from_path(psl_filename)
    else:
        warnings.warn("Cannot find Public_suffix_list.dat. you must import it with \"psl.load_from_path\" or \"psl.load_from_string\" or \"psl.update\" functions",
                      RuntimeWarning, stacklevel=2)

