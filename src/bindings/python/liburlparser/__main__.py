#!/bin/python3
from __future__ import annotations

import argparse
import sys

from . import Host, Url, __doc__, __version__, get_cmake_dir, get_include_dir, get_lib_dir, utils


def show_if_not_none(_str, _class, _parts):
    if _str is not None:
        parsed = _class(_str)
        parsed_dict = utils.to_dict(parsed, flatten=True)
        try:
            output_string = " ".join([parsed_dict[part] for part in _parts]) if _parts else parsed.to_json()
        except KeyError as e:
            sys.stderr.write(f"Error: Invalid part '{e.args[0]}' specified\n")
            sys.exit(1)
        sys.stdout.write(output_string)


def main(args):
    # Build-system helpers (same idea as `python -m nanobind --cmake_dir`):
    # print a path and exit, so a CMakeLists.txt / Makefile can locate the
    # shipped C++ package without importing anything itself.
    for flag, getter in ((args.cmake_dir, get_cmake_dir),
                         (args.include_dir, get_include_dir),
                         (args.lib_dir, get_lib_dir)):
        if flag:
            print(getter())
            return
    if args.url and args.host:
        sys.stderr.write("Error: Either --url or --host argument must be provided\n")
        exit(1)
    show_if_not_none(args.url, Url, args.parts)
    # Host classifies --host as a Hostname, IPv4, or IPv6 (rather than
    # always treating it as a literal domain name the way the Hostname
    # constructor alone would).
    show_if_not_none(args.host, Host, args.parts)
    if args.version:
        sys.stdout.write(__version__)
    if args.doc:
        sys.stdout.write(__doc__)


def cli():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", type=str, default=None,
                        help="enter entire url (for example: \"https://google.com/about\")")
    parser.add_argument("--host", type=str, default=None,
                        help="enter just host part of url (for example: \"google.com\")")
    parser.add_argument('-v', '--version', action='store_true', help="showing version of module")
    parser.add_argument('--parts', type=str, nargs='+', help="list of parts to display")
    parser.add_argument('--doc', action='store_true', help="showing version of module")
    parser.add_argument("--cmake-dir", "--cmake_dir", dest="cmake_dir", action="store_true",
                        help="print the directory containing liburlparserConfig.cmake")
    parser.add_argument("--include-dir", "--include_dir", dest="include_dir", action="store_true",
                        help="print the directory containing urlparser.h")
    parser.add_argument("--lib-dir", "--lib_dir", dest="lib_dir", action="store_true",
                        help="print the directory containing the compiled C++ library")
    args = parser.parse_args(args=None if sys.argv[1:] else ['--help'])
    main(args)


if __name__ == '__main__':
    cli()
