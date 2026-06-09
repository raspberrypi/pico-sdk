#!/usr/bin/env python3
#
# Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for CMake build defines
# Outputs a tab separated file of the configuration item:
# name	location	platform	chip	description	type	default	group
#
# Usage:
#
# tools/extract_build_defines.py <root of repo> [output file]
#
# If not specified, output file will be `pico_build_defines.tsv`


import os
import sys
import re
import csv
import logging

from collections import defaultdict

if sys.version_info < (3, 11):
    # Python <3.11 doesn't have ExceptionGroup, so define a simple one
    class ExceptionGroup(Exception):
        def __init__(self, message, errors):
            message += "\n" + "\n".join(str(e) for e in errors)
            super().__init__(message)

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

scandir = sys.argv[1]
outfile = sys.argv[2] if len(sys.argv) > 2 else 'pico_build_defines.tsv'

BASE_CONFIG_NAME = 'PICO_CONFIG'
BASE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CONFIG_NAME))
BASE_CMAKE_CONFIG_NAME = 'PICO_CMAKE_CONFIG'
BASE_CMAKE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CMAKE_CONFIG_NAME))
BASE_BUILD_DEFINE_NAME = 'PICO_BUILD_DEFINE'
BASE_BUILD_DEFINE_RE = re.compile(r'\b{}\b'.format(BASE_BUILD_DEFINE_NAME))

BUILD_DEFINE_RE = re.compile(r'#\s+{}:\s+(\w+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_BUILD_DEFINE_NAME))

PROPERTY_TYPE = 'type'
PROPERTY_DEFAULT = 'default'
PROPERTY_MIN = 'min'
PROPERTY_MAX = 'max'
PROPERTY_GROUP = 'group'
ALLOWED_CONFIG_PROPERTIES = set([PROPERTY_TYPE, PROPERTY_DEFAULT, PROPERTY_MIN, PROPERTY_MAX, PROPERTY_GROUP])

PROPERTY_TYPE_INT = 'int'
PROPERTY_TYPE_BOOL = 'bool'
PROPERTY_TYPE_STRING = 'string'
PROPERTY_TYPE_LIST = 'list'

CHIP_NAMES = ["rp2040", "rp2350"]

chips_all_configs = defaultdict(dict)
all_attrs = set()
chips_all_descriptions = defaultdict(dict)



def ValidateAttrs(config_name, config_attrs, file_path, linenum):
    type_str = config_attrs.get(PROPERTY_TYPE)
    errors = []

    # Validate attrs
    for key in config_attrs.keys():
        if key not in ALLOWED_CONFIG_PROPERTIES:
            errors.append(Exception('{} at {}:{} has unexpected property "{}"'.format(config_name, file_path, linenum, key)))

    str_values = dict()
    parsed_values = dict()
    if type_str == PROPERTY_TYPE_INT:
        for attr_name in (PROPERTY_MIN, PROPERTY_MAX, PROPERTY_DEFAULT):
            str_values[attr_name] = config_attrs.get(attr_name, None)
            if str_values[attr_name] is not None:
                try:
                    m = re.match(r'^(\d+)e(\d+)$', str_values[attr_name].lower())
                    if m:
                        parsed_values[attr_name] = int(m.group(1)) * 10**int(m.group(2))
                    else:
                        parsed_values[attr_name] = int(str_values[attr_name], 0)
                except ValueError:
                    logger.info('{} at {}:{} has non-integer {} value "{}"'.format(config_name, file_path, linenum, attr_name, str_values[attr_name]))
        for (small_attr, large_attr) in (
            (PROPERTY_MIN, PROPERTY_MAX),
            (PROPERTY_MIN, PROPERTY_DEFAULT),
            (PROPERTY_DEFAULT, PROPERTY_MAX),
        ):
            if small_attr in parsed_values and large_attr in parsed_values and parsed_values[small_attr] > parsed_values[large_attr]:
                errors.append(Exception('{} at {}:{} has {} {} > {} {}'.format(config_name, file_path, linenum, small_attr, str_values[small_attr], large_attr, str_values[large_attr])))

    elif type_str == PROPERTY_TYPE_BOOL:
        assert PROPERTY_MIN not in config_attrs
        assert PROPERTY_MAX not in config_attrs

        attr_name = PROPERTY_DEFAULT
        str_values[attr_name] = config_attrs.get(attr_name, None)
        if str_values[attr_name] is not None:
            if (str_values[attr_name] not in ('0', '1')) and (str_values[attr_name] not in all_config_names):
                logger.info('{} at {}:{} has non-integer {} value "{}"'.format(config_name, file_path, linenum, attr_name, str_values[attr_name]))

    elif type_str == PROPERTY_TYPE_STRING:
        assert PROPERTY_MIN not in config_attrs
        assert PROPERTY_MAX not in config_attrs

        attr_name = PROPERTY_DEFAULT
        str_values[attr_name] = config_attrs.get(attr_name, None)

    elif type_str == PROPERTY_TYPE_LIST:
        assert PROPERTY_MIN not in config_attrs
        assert PROPERTY_MAX not in config_attrs

        attr_name = PROPERTY_DEFAULT
        str_values[attr_name] = config_attrs.get(attr_name, None)

    else:
        errors.append(Exception("Found unknown {} type {} at {}:{}".format(BASE_BUILD_DEFINE_NAME, type_str, file_path, linenum)))

    return errors

errors = []

# Scan all CMakeLists.txt and .cmake files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    for filename in filenames:
        file_ext = os.path.splitext(filename)[1]
        if filename == 'CMakeLists.txt' or file_ext == '.cmake':
            file_path = os.path.join(dirpath, filename)
            applicable = "all"
            for chip in (*CHIP_NAMES, "host"):
                if "/{}/".format(chip) in dirpath:
                    applicable = chip
                    break

            with open(file_path, encoding="ISO-8859-1") as fh:
                linenum = 0
                for line in fh.readlines():
                    linenum += 1
                    line = line.strip()
                    if BASE_CONFIG_RE.search(line):
                        errors.append(Exception("Found {} at {}:{} ({}) which isn't expected in {} files".format(BASE_CONFIG_NAME, file_path, linenum, line, filename if filename == 'CMakeLists.txt' else file_ext)))
                    elif BASE_BUILD_DEFINE_RE.search(line):
                        m = BUILD_DEFINE_RE.match(line)
                        if not m:
                            if re.match(r"^\s*#\s*# ", line):
                                logger.info("Possible misformatted {} at {}:{} ({})".format(BASE_BUILD_DEFINE_NAME, file_path, linenum, line))
                            else:
                                errors.append(Exception("Found misformatted {} at {}:{} ({})".format(BASE_BUILD_DEFINE_NAME, file_path, linenum, line)))
                        else:
                            config_name = m.group(1)
                            config_description = m.group(2)
                            _attrs = m.group(3)
                            # allow commas to appear inside brackets by converting them to and from NULL chars
                            _attrs = re.sub(r'(\(.+\))', lambda m: m.group(1).replace(',', '\0'), _attrs)

                            if '=' in config_description and not '==' in config_description:
                                errors.append(Exception("For {} at {}:{} the description was set to '{}' - has the description field been omitted?".format(config_name, file_path, linenum, config_description)))
                            all_descriptions = chips_all_descriptions[applicable]
                            if config_description in all_descriptions:
                                errors.append(Exception("Found description {} at {}:{} but it was already used at {}:{}".format(config_description, file_path, linenum, os.path.join(scandir, all_descriptions[config_description]['filename']), all_descriptions[config_description]['line_number'])))
                            else:
                                all_descriptions[config_description] = {'config_name': config_name, 'filename': os.path.relpath(file_path, scandir), 'line_number': linenum}

                            config_attrs = {}
                            prev = None
                            # Handle case where attr value contains a comma
                            for item in _attrs.split(','):
                                if "=" not in item:
                                    assert(prev)
                                    item = prev + "," + item
                                try:
                                    k, v = (i.strip() for i in item.split('='))
                                except ValueError:
                                    errors.append(Exception('{} at {}:{} has malformed value {}'.format(config_name, file_path, linenum, item)))
                                config_attrs[k] = v.replace('\0', ',')
                                all_attrs.add(k)
                                prev = item
                            #print(file_path, config_name, config_attrs)

                            if 'group' not in config_attrs:
                                errors.append(Exception('{} at {}:{} has no group attribute'.format(config_name, file_path, linenum)))

                            #print(file_path, config_name, config_attrs)
                            all_configs = chips_all_configs[applicable]
                            if config_name in all_configs:
                                errors.append(Exception("Found {} at {}:{} but it was already declared at {}:{}".format(config_name, file_path, linenum, os.path.join(scandir, all_configs[config_name]['filename']), all_configs[config_name]['line_number'])))
                            else:
                                all_configs[config_name] = {'attrs': config_attrs, 'filename': os.path.relpath(file_path, scandir), 'line_number': linenum, 'description': config_description}


all_config_names = set()
for all_configs in chips_all_configs.values():
    all_config_names.update(all_configs.keys())

for applicable, all_configs in chips_all_configs.items():
    for config_name, config_obj in all_configs.items():
        file_path = os.path.join(scandir, config_obj['filename'])
        linenum = config_obj['line_number']

        errors.extend(ValidateAttrs(config_name, config_obj['attrs'], file_path, linenum))

# All settings in "host" should also be in "all"
for config_name, config_obj in chips_all_configs["host"].items():
    if config_name not in chips_all_configs["all"]:
        file_path = os.path.join(scandir, config_obj['filename'])
        linenum = config_obj['line_number']
        errors.append(Exception("Found 'host' config {} at {}:{}, but no matching non-host config found".format(config_name, file_path, linenum)))

# Any chip-specific settings should not be in "all"
for chip in CHIP_NAMES:
    for config_name, chip_config_obj in chips_all_configs[chip].items():
        if config_name in chips_all_configs["all"]:
            all_config_obj = chips_all_configs["all"][config_name]
            chip_file_path = os.path.join(scandir, chip_config_obj['filename'])
            chip_linenum = chip_config_obj['line_number']
            all_file_path = os.path.join(scandir, all_config_obj['filename'])
            all_linenum = all_config_obj['line_number']
            errors.append(Exception("'{}' config {} at {}:{} also found at {}:{}".format(chip, config_name, chip_file_path, chip_linenum, all_file_path, all_linenum)))

def build_mismatch_exception_message(name, thing, config_obj1, value1, config_obj2, value2):
    obj1_filepath = os.path.join(scandir, config_obj1['filename'])
    obj2_filepath = os.path.join(scandir, config_obj2['filename'])
    return "'{}' {} mismatch at {}:{} ({}) and {}:{} ({})".format(name, thing, obj1_filepath, config_obj1['line_number'], value1, obj2_filepath, config_obj2['line_number'], value2)

# Check that any identically-named setttings have appropriate matching attributes
for applicable in chips_all_configs:
    for other in chips_all_configs:
        if other == applicable:
            continue
        for config_name, applicable_config_obj in chips_all_configs[applicable].items():
            if config_name in chips_all_configs[other]:
                other_config_obj = chips_all_configs[other][config_name]
                # Check that fields match
                for field in ['description']:
                    applicable_value = applicable_config_obj[field]
                    other_value = other_config_obj[field]
                    if applicable_value != other_value:
                        errors.append(Exception(build_mismatch_exception_message(config_name, field, applicable_config_obj, applicable_value, other_config_obj, other_value)))
                # Check that attributes match
                for attr in applicable_config_obj['attrs']:
                    if attr != 'default': # totally fine for defaults to vary per-platform
                        applicable_value = applicable_config_obj['attrs'][attr]
                        other_value = other_config_obj['attrs'][attr]
                        if applicable_value != other_value:
                            errors.append(Exception(build_mismatch_exception_message(config_name, "attribute '{}'".format(attr), applicable_config_obj, applicable_value, other_config_obj, other_value)))

# Raise errors if any were found
if errors:
    raise ExceptionGroup("Errors in {}".format(outfile), errors)

# Sort the output alphabetically by name and then by chip
output_rows = set()
for chip in (*CHIP_NAMES, "host", "all"):
    if chip in chips_all_configs:
        all_configs = chips_all_configs[chip]
        for config_name in all_configs:
            output_rows.add((config_name, chip))

with open(outfile, 'w', newline='') as csvfile:
    fieldnames = ('name', 'location', 'platform', 'chip', 'description', 'type') + tuple(sorted(all_attrs - set(['type'])))
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore', dialect='excel-tab')

    writer.writeheader()
    for config_name, chip in sorted(output_rows):
        config_obj = chips_all_configs[chip][config_name]
        writer.writerow({'name': config_name, 'location': '/{}:{}'.format(config_obj['filename'], config_obj['line_number']), 'platform': "host" if chip == "host" else "rp2", 'chip': chip if chip in CHIP_NAMES else "all", 'description': config_obj['description'], **config_obj['attrs']})
