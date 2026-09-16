#!/usr/bin/env python3
"""Generate a version header for the Bazel build.

Splits a semantic version string into major, minor, and patch and uses the
provided template to produce a working version header.
"""

import argparse
import re
import sys


def _parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        "--version-string",
        required=True,
        help="SDK version string",
    )
    parser.add_argument(
        "--template",
        type=argparse.FileType("r"),
        required=True,
        help="Path to version.h.in",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=argparse.FileType("wb"),
        default=sys.stdout.buffer,
        help="Output file path. Defaults to stdout.",
    )
    return parser.parse_args()


_EXPANSION_REGEX = re.compile(r"(?:\$\{)([a-zA-Z]\w*)(?:\})")


def generate_version_header(version_string, template, output):
    version_parts = version_string.split('.')
    defines = {
        "PICO_SDK_VERSION_MAJOR": version_parts[0],
        "PICO_SDK_VERSION_MINOR": version_parts[1],
        "PICO_SDK_VERSION_REVISION": version_parts[2].split('-')[0],
        "PICO_SDK_VERSION_STRING": version_string,
    }
    output.write(
        _EXPANSION_REGEX.sub(
            lambda val: str(defines.get(val.group(1))),
            template.read(),
        ).encode()
    )


if __name__ == "__main__":
    sys.exit(generate_version_header(**vars(_parse_args())))
