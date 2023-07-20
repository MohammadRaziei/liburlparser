#!/bin/python3
from __future__ import annotations

import warnings
from pathlib import Path

import requests

from ._core import Host, Psl, Url, __doc__, __version__

psl = Psl() # psl

def warning_on_one_line(message, category, filename, lineno, file=None, line=None):
    return f'{filename}:{lineno}: {category.__name__}: {message}\n'
warnings.formatwarning = warning_on_one_line


def psl_update():
    resp = requests.get(psl.url)
    psl.load_from_string(resp.text)

psl.update = psl_update

if not psl.is_loaded():
    psl_filename = Path(__file__).parent / psl.filename
    if psl_filename.exists():
        psl.load_from_path(psl_filename.as_posix())
    else:
        warnings.warn("Cannot find Public_suffix_list.dat. you must import it with \"psl.load_from_path\" or \"psl.load_from_string\" or \"psl.update\" functions",
                      RuntimeWarning, stacklevel=2)

