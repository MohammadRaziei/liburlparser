#!/bin/python3
from __future__ import annotations

import warnings
from pathlib import Path
from urllib.request import urlopen

from filelock import FileLock

from ._core import Host, Psl, Url, __doc__

psl = Psl()  # psl


def warning_on_one_line(message, category, filename, lineno, file=None, line=None):
    return f'{filename}:{lineno}: {category.__name__}: {message}\n'


warnings.formatwarning = warning_on_one_line

def psl_update():
    """Download and load the current Public Suffix List."""
    with urlopen(psl.url) as response:
        psl.load_from_string(response.read().decode("utf-8"))


psl.update = psl_update

if not psl.is_loaded():
    psl_filename = Path(__file__).parent / psl.filename
    if psl_filename.exists():
        with FileLock(psl_filename.with_suffix(".lock")):
            psl.load_from_path(psl_filename.as_posix())
    else:
        warnings.warn(
            f"Cannot find {psl_filename}. you must import it with \"psl.load_from_path\" or \"psl.load_from_string\" or \"psl.update\" functions",
            RuntimeWarning, stacklevel=2)


