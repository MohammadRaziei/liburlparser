#!/bin/python3
from __future__ import annotations

import argparse
import sys

from . import Host, Url, __doc__, __version__
from . import utils


def show_if_not_none(_str, _class, _parts):
    if _str is not None:
        parsed = _class(_str)
        parsed_dict = utils.flatten_dict(parsed.to_dict())
        try:
            output_string = " ".join([parsed_dict[part] for part in _parts]) if _parts else parsed.to_json()
        except KeyError as e:
            sys.stderr.write(f"Error: Invalid part '{e.args[0]}' specified\n")
            sys.exit(1)
        sys.stdout.write(output_string)


def main(args):
    if not args.url and not args.host:
        sys.stderr.write("Error: Either --url or --host argument must be provided\n")
    show_if_not_none(args.url, Url, args.parts)
    show_if_not_none(args.host, Host, args.parts)
    if args.version:
        sys.stdout.write(__version__)
    if args.doc:
        sys.stdout.write(__doc__)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", type=str, default=None,
                        help="enter entire url (for example: \"https://google.com/about\")")
    parser.add_argument("--host", type=str, default=None,
                        help="enter just host part of url (for example: \"google.com\")")
    parser.add_argument('-v', '--version', action='store_true', help="showing version of module")
    parser.add_argument('--parts', type=str, nargs='+', help="list of parts to display")
    parser.add_argument('--doc', action='store_true', help="showing version of module")
    args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
    main(args)
