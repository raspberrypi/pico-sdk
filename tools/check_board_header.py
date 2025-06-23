#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Simple script to check basic validity of a board-header-file
#
# Usage:
#
# tools/check_board_header.py src/boards/include/boards/<board.h>


import re
import sys
import os.path
import json
import warnings

from collections import namedtuple

if sys.version_info < (3, 11):
    # Python <3.11 doesn't have ExceptionGroup, so define a simple one
    class ExceptionGroup(Exception):
        def __init__(self, message, errors):
            message += "\n" + "\n".join(str(e) for e in errors)
            super().__init__(message)


# warnings off by default, because some boards use the same pin for multiple purposes
show_warnings = False

chip_interfaces = {
    'RP2040': "src/rp2040/rp2040_interface_pins.json",
    'RP2350A': "src/rp2350/rp2350a_interface_pins.json",
    'RP2350B': "src/rp2350/rp2350b_interface_pins.json",
}

compulsory_cmake_settings = set(['PICO_PLATFORM'])
compulsory_cmake_default_settings = set(['PICO_FLASH_SIZE_BYTES'])
matching_cmake_default_settings = set(['PICO_FLASH_SIZE_BYTES', 'PICO_RP2350_A2_SUPPORTED'])
compulsory_defines = set(['PICO_FLASH_SIZE_BYTES'])

DefineType = namedtuple("DefineType", ["name", "value", "resolved_value", "lineno"])

def list_to_string_with(lst, joiner):
    elems = len(lst)
    if elems == 0:
        return ""
    elif elems == 1:
        return str(lst[0])
    else:
        return "{} {} {}".format(", ".join(str(l) for l in lst[:-1]), joiner, lst[-1])


board_header = sys.argv[1]
if not os.path.isfile(board_header):
    raise Exception("{} doesn't exist".format(board_header))
board_header_basename = os.path.basename(board_header)

expected_include_suggestion = "/".join(board_header.split("/")[-2:])
expected_include_guard = "_" + re.sub(r"\W", "_", expected_include_suggestion.upper())
expected_board_detection = re.sub(r"\W", "_", expected_include_suggestion.split("/")[-1].upper()[:-2])

defines = dict()
cmake_settings = dict()
cmake_default_settings = dict()

has_include_guard = False
has_board_detection = False
has_include_suggestion = False


def read_defines_from(header_file, defines_dict):
    errors = []
    with open(header_file) as fh:
        last_ifndef = None
        last_ifndef_lineno = -1
        validity_stack = [True]
        board_detection_is_next = False
        for lineno, line in enumerate(fh.readlines()):
            lineno += 1
            # strip trailing comments
            line = re.sub(r"(?<=\S)\s*//.*$", "", line)

            # look for "// old_comment BLAH_BLAH=42" and suggest changing it to "new_macro(BLAH_BLAH, 42)"
            for (old_comment, new_macro) in (
                ('pico_cmake_set', 'pico_board_cmake_set'),
                ('pico_cmake_set_default', 'pico_board_cmake_set_default')
            ):
                m = re.match(r"^\s*//\s*{}\s+(\w+)\s*=\s*(.+?)\s*$".format(old_comment), line)
                if m:
                    name = m.group(1)
                    value = m.group(2)
                    errors.append(Exception("{}:{}  \"// {} {}={}\" should be replaced with \"{}({}, {})\"".format(board_header, lineno, old_comment, name, value, new_macro, name, value)))

            # look for "pico_board_cmake_set(BLAH_BLAH, 42)"
            m = re.match(r"^\s*pico_board_cmake_set\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(.*)\s*\)\s*$", line)
            if m:
                #print(m.groups())
                name = m.group(1)
                value = m.group(2)
                # check all uppercase
                if name != name.upper():
                    errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
                # check for multiply-defined values
                if name in cmake_settings:
                    if cmake_settings[name].value != value:
                        errors.append(Exception("{}:{}  Conflicting values for pico_board_cmake_set({}) ({} and {})".format(board_header, lineno, name, cmake_settings[name].value, value)))
                    else:
                        if show_warnings:
                            warnings.warn("{}:{}  Multiple values for pico_board_cmake_set({}) ({} and {})".format(board_header, lineno, name, cmake_settings[name].value, value))
                else:
                   cmake_settings[name] = DefineType(name, value, None, lineno)
                continue

            # look for "pico_board_cmake_set_default(BLAH_BLAH, 42)"
            m = re.match(r"^\s*pico_board_cmake_set_default\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(.*)\s*\)\s*$", line)
            if m:
                #print(m.groups())
                name = m.group(1)
                value = m.group(2)
                # check all uppercase
                if name != name.upper():
                    errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
                if name not in cmake_default_settings:
                   cmake_default_settings[name] = DefineType(name, value, None, lineno)
                continue

            # look for "#else"
            m = re.match(r"^\s*#else\s*$", line)
            if m:
                validity_stack[-1] = not validity_stack[-1]
                continue

            # look for #endif
            m = re.match(r"^\s*#endif\s*$", line)
            if m:
                validity_stack.pop()
                continue

            if validity_stack[-1]:
                # look for "#include "foo.h"
                m = re.match(r"""^\s*#include\s+"(.+?)"\s*$""", line)
                if m:
                    include = m.group(1)
                    #print("Found nested include \"{}\" in {}".format(include, header_file))
                    assert include.endswith(".h")
                    # assume that the include is also in the boards directory
                    assert "/" not in include or include.startswith("boards/")
                    errors.extend(read_defines_from(os.path.join(os.path.dirname(board_header), os.path.basename(include)), defines))
                    continue

                # look for "#if BLAH_BLAH"
                m = re.match(r"^\s*#if\s+(\w+)\s*$", line)
                if m:
                    last_if = m.group(1)
                    last_if_lineno = lineno
                    validity_stack.append(bool(defines[last_if].resolved_value))
                    continue

                # look for "#ifdef BLAH_BLAH"
                m = re.match(r"^\s*#ifdef\s+(\w+)\s*$", line)
                if m:
                    last_ifdef = m.group(1)
                    last_ifdef_lineno = lineno
                    validity_stack.append(last_ifdef in defines)
                    continue

                # look for "#ifndef BLAH_BLAH"
                m = re.match(r"^\s*#ifndef\s+(\w+)\s*$", line)
                if m:
                    last_ifndef = m.group(1)
                    last_ifndef_lineno = lineno
                    validity_stack.append(last_ifndef not in defines)
                    continue

                # look for "#define BLAH_BLAH" or "#define BLAH_BLAH 42"
                m = re.match(r"^\s*#define\s+(\w+)(?:\s+(.+?))?\s*$", line)
                if m:
                    #print(m.groups())
                    name = m.group(1)
                    value = m.group(2)
                    # check all uppercase
                    if name != name.upper():
                        errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
                    # check that adjacent #ifndef and #define lines match up
                    if last_ifndef_lineno + 1 == lineno:
                        if last_ifndef != name:
                            errors.append(Exception("{}:{}  #ifndef {} / #define {} mismatch".format(board_header, last_ifndef_lineno, last_ifndef, name)))
                    if value:
                        try:
                            # most board-defines are integer values
                            value = int(value, 0)
                        except ValueError:
                            pass

                        # resolve nested defines
                        resolved_value = value
                        while resolved_value in defines_dict:
                            resolved_value = defines_dict[resolved_value].resolved_value
                    else:
                        resolved_value = None

                    # check for multiply-defined values
                    if name in defines_dict:
                        if defines_dict[name].value != value:
                            errors.append(Exception("{}:{}  Conflicting definitions for {} ({} and {})".format(board_header, lineno, name, defines_dict[name].value, value)))
                        else:
                            if show_warnings:
                                warnings.warn("{}:{}  Multiple definitions for {} ({} and {})".format(board_header, lineno, name, defines_dict[name].value, value))
                    else:
                        defines_dict[name] = DefineType(name, value, resolved_value, lineno)
    return errors


if board_header_basename == "amethyst_fpga.h":
    defines['PICO_RP2350'] = DefineType('PICO_RP2350', 1, 1, -1)
    defines['PICO_RP2350A'] = DefineType('PICO_RP2350A', 0, 0, -1)

errors = []

with open(board_header) as header_fh:
    last_ifndef = None
    last_ifndef_lineno = -1
    validity_stack = [True]
    board_detection_is_next = False
    for lineno, line in enumerate(header_fh.readlines()):
        lineno += 1
        # strip trailing comments
        line = re.sub(r"(?<=\S)\s*//.*$", "", line)

        # look for board-detection comment
        if re.match(r"^\s*// For board detection", line):
            board_detection_is_next = True
            continue

        # check include-suggestion
        m = re.match(r"""^\s*// This header may be included by other board headers as "(.+?)"$""", line)
        if m:
            include_suggestion = m.group(1)
            if include_suggestion == expected_include_suggestion:
                has_include_suggestion = True
            else:
                errors.append(Exception("{}:{}  Suggests including \"{}\" but file is named \"{}\"".format(board_header, lineno, include_suggestion, expected_include_suggestion)))
            continue

        # look for "// old_comment BLAH_BLAH=42" and suggest changing it to "new_macro(BLAH_BLAH, 42)"
        for (old_comment, new_macro) in (
            ('pico_cmake_set', 'pico_board_cmake_set'),
            ('pico_cmake_set_default', 'pico_board_cmake_set_default')
        ):
            m = re.match(r"^\s*//\s*{}\s+(\w+)\s*=\s*(.+?)\s*$".format(old_comment), line)
            if m:
                name = m.group(1)
                value = m.group(2)
                errors.append(Exception("{}:{}  \"// {} {}={}\" should be replaced with \"{}({}, {})\"".format(board_header, lineno, old_comment, name, value, new_macro, name, value)))

        # look for "pico_board_cmake_set(BLAH_BLAH, 42)"
        m = re.match(r"^\s*pico_board_cmake_set\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(.*)\s*\)\s*$", line)
        if m:
            #print(m.groups())
            name = m.group(1)
            value = m.group(2)
            # check all uppercase
            if name != name.upper():
                errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
            # check for multiply-defined values
            if name in cmake_settings:
                errors.append(Exception("{}:{}  Multiple values for pico_board_cmake_set({}) ({} and {})".format(board_header, lineno, name, cmake_settings[name].value, value)))
            else:
                if value:
                    try:
                        # most cmake settings are integer values
                        value = int(value, 0)
                    except ValueError:
                        pass
                cmake_settings[name] = DefineType(name, value, None, lineno)
            continue

        # look for "pico_board_cmake_set_default(BLAH_BLAH, 42)"
        m = re.match(r"^\s*pico_board_cmake_set_default\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(.*)\s*\)\s*$", line)
        if m:
            #print(m.groups())
            name = m.group(1)
            value = m.group(2)
            # check all uppercase
            if name != name.upper():
                errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
            # check for multiply-defined values
            if name in cmake_default_settings:
                errors.append(Exception("{}:{}  Multiple values for pico_board_cmake_set_default({}) ({} and {})".format(board_header, lineno, name, cmake_default_settings[name].value, value)))
            else:
                if value:
                    try:
                        # most cmake settings are integer values
                        value = int(value, 0)
                    except ValueError:
                        pass
                cmake_default_settings[name] = DefineType(name, value, None, lineno)
            continue

        # look for "#else"
        m = re.match(r"^\s*#else\s*$", line)
        if m:
            validity_stack[-1] = not validity_stack[-1]
            continue

        # look for #endif
        m = re.match(r"^\s*#endif\s*$", line)
        if m:
            validity_stack.pop()
            continue

        if validity_stack[-1]:
            # look for "#include "foo.h"
            m = re.match(r"""^\s*#include\s+"(.+?)"\s*$""", line)
            if m:
                include = m.group(1)
                #print("Found include \"{}\" in {}".format(include, board_header))
                assert include.endswith(".h")
                # assume that the include is also in the boards directory
                assert "/" not in include or include.startswith("boards/")
                errors.extend(read_defines_from(os.path.join(os.path.dirname(board_header), os.path.basename(include)), defines))
                continue

            # look for "#if BLAH_BLAH"
            m = re.match(r"^\s*#if\s+(!)?\s*(\w+)\s*$", line)
            if m:
                valid = bool(defines[m.group(2)].resolved_value)
                if m.group(1):
                    valid = not valid
                validity_stack.append(valid)
                continue

            # look for "#ifdef BLAH_BLAH"
            m = re.match(r"^\s*#ifdef\s+(\w+)\s*$", line)
            if m:
                validity_stack.append(m.group(1) in defines)
                continue

            # look for "#ifndef BLAH_BLAH"
            m = re.match(r"^\s*#ifndef\s+(\w+)\s*$", line)
            if m:
                last_ifndef = m.group(1)
                last_ifndef_lineno = lineno
                validity_stack.append(last_ifndef not in defines)
                continue

            # look for "#define BLAH_BLAH" or "#define BLAH_BLAH 42"
            m = re.match(r"^\s*#define\s+(\w+)(?:\s+(.+?))?\s*$", line)
            if m:
                #print(m.groups())
                name = m.group(1)
                value = m.group(2)
                # check all uppercase
                if name != name.upper():
                    errors.append(Exception("{}:{}  Expected \"{}\" to be all uppercase".format(board_header, lineno, name)))
                # check that adjacent #ifndef and #define lines match up
                if last_ifndef_lineno + 1 == lineno:
                    if last_ifndef != name:
                        errors.append(Exception("{}:{}  #ifndef {} / #define {} mismatch".format(board_header, last_ifndef_lineno, last_ifndef, name)))
                if value:
                    try:
                        # most board-defines are integer values
                        value = int(value, 0)
                    except ValueError:
                        pass

                    # resolve nested defines
                    resolved_value = value
                    while resolved_value in defines:
                        resolved_value = defines[resolved_value].resolved_value
                else:
                    resolved_value = None

                # check the include-guard define
                if re.match(r"^_BOARDS_(\w+)_H$", name):
                    # check it has an #ifndef
                    if last_ifndef_lineno +1 != lineno:
                        errors.append(Exception("{}:{}  Include-guard #define {} is missing an #ifndef".format(board_header, lineno, name)))
                    if value:
                        errors.append(Exception("{}:{}  Include-guard #define {} shouldn't have a value".format(board_header, lineno, name)))
                    if any(defines[d].lineno >= 0 for d in defines):
                        errors.append(Exception("{}:{}  Include-guard #define {} should be the first define".format(board_header, lineno, name)))
                    if name == expected_include_guard:
                        has_include_guard = True
                    else:
                        errors.append(Exception("{}:{}  Found include-guard #define {} but expected {}".format(board_header, lineno, name, expected_include_guard)))
                # check board-detection define
                if board_detection_is_next:
                    board_detection_is_next = False
                    if value:
                        errors.append(Exception("{}:{}  Board-detection #define {} shouldn't have a value".format(board_header, lineno, name)))
                    # this is a bit messy because pico.h does "#define RASPBERRYPI_PICO" and metrotech_xerxes_rp2040.h does "#define XERXES_RP2040"
                    if name.endswith(expected_board_detection) or expected_board_detection.endswith(name):
                        has_board_detection = True
                    else:
                        errors.append(Exception("{}:{}  Board-detection #define {} should end with {}".format(board_header, lineno, name, expected_board_detection)))
                # check for multiply-defined values
                if name in defines:
                    errors.append(Exception("{}:{}  Multiple definitions for {} ({} and {})".format(board_header, lineno, name, defines[name].value, value)))
                else:
                    defines[name] = DefineType(name, value, resolved_value, lineno)
                continue


#import pprint; pprint.pprint(dict(sorted(defines.items(), key=lambda x: x[1].lineno)))
#import pprint; pprint.pprint(dict(sorted(cmake_settings.items(), key=lambda x: x[1].lineno)))
#import pprint; pprint.pprint(dict(sorted(cmake_default_settings.items(), key=lambda x: x[1].lineno)))
chip = None
if board_header_basename == "none.h":
    chip = 'RP2040'
    other_chip = 'RP2350'
else:
    for setting in compulsory_cmake_settings:
        if setting not in cmake_settings:
            errors.append(Exception("{} is missing a pico_board_cmake_set({}, XXX) call".format(board_header, setting)))

    # Must be raised before continuing, in case compulsory settings are missing
    if errors:
        raise ExceptionGroup("Errors in {}".format(board_header), errors)
    if cmake_settings['PICO_PLATFORM'].value == "rp2040":
        chip = 'RP2040'
        other_chip = 'RP2350'
    elif cmake_settings['PICO_PLATFORM'].value == "rp2350":
        other_chip = 'RP2040'
        if 'PICO_RP2350B' in defines:
            errors.append(Exception("{} sets #define {} {} (should probably be #define {} {})".format(board_header, 'PICO_RP2350B', defines['PICO_RP2350B'].resolved_value, 'PICO_RP2350A', 1 - defines['PICO_RP2350B'].resolved_value)))
        if 'PICO_RP2350A' not in defines:
            errors.append(Exception("{} has no #define for {} (set to 1 for RP2350A, or 0 for RP2350B)".format(board_header, 'PICO_RP2350A')))
        else:
            if defines['PICO_RP2350A'].resolved_value == 1:
                chip = 'RP2350A'
            else:
                chip = 'RP2350B'
        if not board_header.endswith("amethyst_fpga.h"):
            if 'PICO_RP2350_A2_SUPPORTED' not in cmake_default_settings:
                errors.append(Exception("{} uses chip {} but is missing a pico_board_cmake_set_default({}, XXX) call".format(board_header, chip, 'PICO_RP2350_A2_SUPPORTED')))
            if 'PICO_RP2350_A2_SUPPORTED' not in defines:
                errors.append(Exception("{} uses chip {} but is missing a #define {}".format(board_header, chip, 'PICO_RP2350_A2_SUPPORTED')))
            elif defines['PICO_RP2350_A2_SUPPORTED'].resolved_value != 1:
                errors.append(Exception("{} sets #define {} {} (should be 1)".format(board_header, chip, 'PICO_RP2350_A2_SUPPORTED', defines['PICO_RP2350_A2_SUPPORTED'].resolved_value)))
    for setting in compulsory_cmake_default_settings:
        if setting not in cmake_default_settings:
            errors.append(Exception("{} is missing a pico_board_cmake_set_default({}, XXX) call".format(board_header, setting)))
    for setting in matching_cmake_default_settings:
        if setting in cmake_default_settings and setting not in defines:
            errors.append(Exception("{} has pico_board_cmake_set_default({}, XXX) but is missing a matching #define".format(board_header, setting)))
        elif setting in defines and setting not in cmake_default_settings:
            errors.append(Exception("{} has #define {} but is missing a matching pico_board_cmake_set_default({}, XXX) call".format(board_header, setting, setting)))
        elif setting in defines and setting in cmake_default_settings:
            if cmake_default_settings[setting].value != defines[setting].resolved_value:
                errors.append(Exception("{} has mismatched pico_board_cmake_set_default and #define values for {}".format(board_header, setting)))
    for setting in compulsory_defines:
        if setting not in defines:
            errors.append(Exception("{} is missing a #define {}".format(board_header, setting)))

if chip is None:
    errors.append(Exception("Couldn't determine chip for {}".format(board_header)))

# Must be raised before continuing, in case chip is not determined
if errors:
    raise ExceptionGroup("Errors in {}".format(board_header), errors)

interfaces_json = chip_interfaces[chip]
if not os.path.isfile(interfaces_json):
    raise Exception("{} doesn't exist".format(interfaces_json))

with open(interfaces_json) as interfaces_fh:
    interface_pins = json.load(interfaces_fh)
    allowed_interfaces = interface_pins["interfaces"]
    allowed_pins = set(interface_pins["pins"])
    # convert instance-keys to integers (allowed by Python but not by JSON)
    for interface in allowed_interfaces:
        instances = allowed_interfaces[interface]["instances"]
        # can't modify a list that we're iterating over, so iterate over a copy
        instances_copy = list(instances)
        for instance in instances_copy:
            instance_num = int(instance)
            instances[instance_num] = instances.pop(instance)

pins = dict() # dict of lists
for name, define in defines.items():

    # check for other-chip defines
    if other_chip in name:
        errors.append(Exception("{}:{}  Header is for {} and so shouldn't have settings for {} ({})".format(board_header, define.lineno, chip, other_chip, name)))

    # check for pin-conflicts
    if name.endswith("_PIN"):
        if define.resolved_value is None:
            errors.append(Exception("{}:{}  {} is set to an undefined value".format(board_header, define.lineno, name)))
        elif not isinstance(define.resolved_value, int):
            errors.append(Exception("{}:{}  {} resolves to a non-integer value {}".format(board_header, define.lineno, name, define.resolved_value)))
        else:
            if define.resolved_value in pins and define.resolved_value == define.value:
                if show_warnings:
                    warnings.warn("{}:{}  Both {} and {} claim to be pin {}".format(board_header, define.lineno, pins[define.resolved_value][0].name, name, define.resolved_value))
                pins[define.resolved_value].append(define)
            else:
                if define.resolved_value not in allowed_pins:
                    errors.append(Exception("{}:{}  Pin {} for {} isn't a valid pin-number".format(board_header, define.lineno, define.resolved_value, name)))
                pins[define.resolved_value] = [define]

    # check for invalid DEFAULT mappings
    m = re.match("^(PICO_DEFAULT_([A-Z0-9]+))_([A-Z0-9]+)_PIN$", name)
    if m:
        instance_name = m.group(1)
        interface = m.group(2)
        function = m.group(3)
        if interface == "WS2812":
            continue
        if interface not in allowed_interfaces:
            errors.append(Exception("{}:{}  {} is defined but {} isn't in {}".format(board_header, define.lineno, name, interface, interfaces_json)))
            continue
        if instance_name not in defines:
            errors.append(Exception("{}:{}  {} is defined but {} isn't defined".format(board_header, define.lineno, name, instance_name)))
            continue
        instance_define = defines[instance_name]
        instance_num = instance_define.resolved_value
        if instance_num not in allowed_interfaces[interface]["instances"]:
            errors.append(Exception("{}:{}  {} is set to an invalid instance {}".format(board_header, instance_define.lineno, instance_define, instance_num)))
            continue
        interface_instance = allowed_interfaces[interface]["instances"][instance_num]
        if function not in interface_instance:
            errors.append(Exception("{}:{}  {} is defined but {} isn't a valid function for {}".format(board_header, define.lineno, name, function, instance_define)))
            continue
        if define.resolved_value not in interface_instance[function]:
            errors.append(Exception("{}:{}  {} is set to {} which isn't a valid pin for {} on {} {}".format(board_header, define.lineno, name, define.resolved_value, function, interface, instance_num)))

    # check that each used DEFAULT interface includes (at least) the expected pin-functions
    m = re.match("^PICO_DEFAULT_([A-Z0-9]+)$", name)
    if m:
        interface = m.group(1)
        if interface not in allowed_interfaces:
            errors.append(Exception("{}:{}  {} is defined but {} isn't in {}".format(board_header, define.lineno, name, interface, interfaces_json)))
            continue
        if "expected_functions" in allowed_interfaces[interface]:
            expected_functions = allowed_interfaces[interface]["expected_functions"]
            if "required" in expected_functions:
                for function in expected_functions["required"]:
                    expected_function_pin = "{}_{}_PIN".format(name, function)
                    if expected_function_pin not in defines:
                        errors.append(Exception("{}:{}  {} is defined but {} isn't defined".format(board_header, define.lineno, name, expected_function_pin)))
            if "one_of" in expected_functions:
                expected_function_pins = list("{}_{}_PIN".format(name, function) for function in expected_functions["one_of"])
                if not any(func_pin in defines for func_pin in expected_function_pins):
                    errors.append(Exception("{}:{}  {} is defined but none of {} are defined".format(board_header, define.lineno, name, list_to_string_with(expected_function_pins, "or"))))

if not has_include_guard:
    errors.append(Exception("{} has no include-guard (expected {})".format(board_header, expected_include_guard)))
if not has_board_detection and expected_board_detection != "NONE":
    errors.append(Exception("{} has no board-detection #define (expected {})".format(board_header, expected_board_detection)))
# lots of headers don't have this
#if not has_include_suggestion:
#    raise Exception("{} has no include-suggestion (expected {})".format(board_header, expected_include_suggestion))

if errors:
    raise ExceptionGroup("Errors in {}".format(board_header), errors)

# Check that #if / #ifdef / #ifndef / #else / #endif are correctly balanced
assert len(validity_stack) == 1 and validity_stack[0]
