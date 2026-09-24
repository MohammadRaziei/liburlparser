#!/bin/python3
from __future__ import annotations

import warnings

from ._urlparser_py import Host, Hostname, IPv4, IPv6, Psl, ScpUrl, Url, __doc__, __version__, quote, unquote, resolve

# Alias: most callers reaching for ScpUrl are parsing a git remote
# specifically - the class itself is named for the underlying address
# *syntax* (shared with scp/rsync/sshfs too), not tied to git.
GitUrl = ScpUrl

psl = Psl.instance()  # psl - already loaded from the data embedded into the compiled extension


def warning_on_one_line(message, category, filename, lineno, file=None, line=None):
    return f'{filename}:{lineno}: {category.__name__}: {message}\n'


warnings.formatwarning = warning_on_one_line

try:
    from urllib.error import URLError  # noqa: F401
    from urllib.request import urlopen

    def psl_update():
        with urlopen(psl.url) as resp:
            charset = resp.headers.get_content_charset() or "utf-8"
            text = resp.read().decode(charset)
        psl.load_from_string(text)

    psl.update = psl_update

except ImportError:
    def psl_update():
        raise NotImplementedError

    psl.update = psl_update

if not psl.is_loaded():
    warnings.warn(
        "PSL data embedded in the compiled extension failed to load. "
        "You can load it manually with \"psl.load_from_path\", \"psl.load_from_string\" or \"psl.update\".",
        RuntimeWarning, stacklevel=2)
