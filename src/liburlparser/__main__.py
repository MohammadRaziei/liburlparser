#!/bin/python3
from __future__ import annotations

import argparse
import sys

from . import Host, Url, __doc__, __version__


def show_if_not_none(_str, _class):
    if _str is not None:
        print(_class(_str).to_json())


def main(args):
    show_if_not_none(args.url, Url)
    show_if_not_none(args.host, Host)
    if args.version:
        print(__version__)
    if args.doc:
        print(__doc__)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", type=str, default=None,
                        help="enter entire url (for example: \"https://google.com/about\")")
    parser.add_argument("--host", type=str, default=None,
                        help="enter just host part of url (for example: \"google.com\")")
    parser.add_argument('-v', '--version', action='store_true', help="showing version of module")
    parser.add_argument('--doc', action='store_true', help="showing version of module")
    args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
    main(args)
