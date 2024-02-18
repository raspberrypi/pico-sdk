#!/usr/bin/env python3

import argparse
import re
import sys

def _parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        'pico_sdk_version_file',
        type=argparse.FileType('r'),
        help="Path to pico_sdk_version.cmake",
    )
    parser.add_argument(
        '--template',
        type=argparse.FileType('r'),
        required=True,
        help="Path to version.h.in",
    )
    parser.add_argument(
        '-o',
        '--output',
        type=argparse.FileType('w'),
        default=sys.stdout.buffer,
        help="Output file path. Defaults to stdout.",
    )
    return parser.parse_args()

_EXPANSION_REGEX = re.compile('(?:\$\{)([a-zA-Z]\w*)(?:\})')
_SET_REGEX = re.compile('(?:set\()([a-zA-Z]\w*)[ ](.*)(?:\))')
_COND_REGEX = re.compile('(?:if\s+\()([a-zA-Z]\w*)(?:\))')
_END_COND_REGEX = re.compile('(?:endif\(\))')

class RudimentaryCmakeParser:
    def __init__(self, in_file):
        self._condition_stack = []
        self._known_vars = {}
        
        for line in in_file.readlines():
            line = line.split('#')[0]
            self._check_for_condition(line)
            self._check_for_condition_end(line)
            self._check_for_define(line)

    def _expand_str(self, contents):
        return _EXPANSION_REGEX.sub(lambda val: str(self._known_vars.get(val.group(1))), contents)

    def _eval_value(self, value):
        value = self._expand_str(value.strip('"'))
        typed_value = value
        try:
            typed_value = float(value)
        except ValueError:
            pass
        try:
            typed_value = int(value)
        except ValueError:
            pass
        
        return typed_value

    def _check_for_condition(self, line):
        maybe_cond = _COND_REGEX.search(line)
        if maybe_cond:
            self._condition_stack.append(bool(self._known_vars.get(maybe_cond.group(1))))

    def _check_for_condition_end(self, line):
        maybe_cond = _END_COND_REGEX.search(line)
        if maybe_cond:
            del self._condition_stack[-1]
    
    def _check_for_define(self, line):
        maybe_var = _SET_REGEX.search(line)
        if maybe_var and (not self._condition_stack or self._condition_stack[-1]):
            key, value = maybe_var.groups()
            self._known_vars[key] = self._eval_value(value)


    def defines(self):
        return self._known_vars


def generate_version_header(pico_sdk_version_file, template, output):
    defines = RudimentaryCmakeParser(pico_sdk_version_file).defines()
    output.write(
        _EXPANSION_REGEX.sub(
            lambda val: str(defines.get(val.group(1))),
            template.read(),
        ).encode()
    )


if __name__ == "__main__":
    sys.exit(generate_version_header(**vars(_parse_args())))
