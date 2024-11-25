#!/usr/bin/env python3
#
# Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for CMake configuration items
# Outputs a tab separated file of the configuration item:
# name	location	platform	chip	description	type	advanced	default	docref	group
#
# Usage:
#
# tools/extract_cmake_configs.py <root of repo> [output file]
#
# If not specified, output file will be `pico_cmake_configs.tsv`


import os
import sys
import re
import csv
import logging

from collections import defaultdict

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

scandir = sys.argv[1]
outfile = sys.argv[2] if len(sys.argv) > 2 else 'pico_cmake_configs.tsv'

BASE_CONFIG_NAME = 'PICO_CONFIG'
BASE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CONFIG_NAME))
BASE_CMAKE_CONFIG_NAME = 'PICO_CMAKE_CONFIG'
BASE_CMAKE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CMAKE_CONFIG_NAME))
BASE_BUILD_DEFINE_NAME = 'PICO_BUILD_DEFINE'
BASE_BUILD_DEFINE_RE = re.compile(r'\b{}\b'.format(BASE_BUILD_DEFINE_NAME))

CMAKE_CONFIG_RE = re.compile(r'#\s+{}:\s+(\w+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_CMAKE_CONFIG_NAME))

ALLOWED_CONFIG_PROPERTIES = set(['type', 'default', 'min', 'max', 'group', 'advanced', 'docref'])

CHIP_NAMES = ["rp2040", "rp2350"]

chips_all_configs = defaultdict(dict)
all_attrs = set()
chips_all_descriptions = defaultdict(dict)



def ValidateAttrs(config_name, config_attrs, file_path, linenum):
    _type = config_attrs.get('type')

    # Validate attrs
    for key in config_attrs.keys():
        if key not in ALLOWED_CONFIG_PROPERTIES:
            raise Exception('{} at {}:{} has unexpected property "{}"'.format(config_name, file_path, linenum, key))

    if _type == 'int':
        _min = _max = _default = None
        if config_attrs.get('min', None) is not None:
            value = config_attrs['min']
            m = re.match(r'^(\d+)e(\d+)$', value.lower())
            if m:
                _min = int(m.group(1)) * 10**int(m.group(2))
            else:
                _min = int(value, 0)
        if config_attrs.get('max', None) is not None:
            value = config_attrs['max']
            m = re.match(r'^(\d+)e(\d+)$', value.lower())
            if m:
                _max = int(m.group(1)) * 10**int(m.group(2))
            else:
                _max = int(value, 0)
        if config_attrs.get('default', None) is not None:
            if '/' not in config_attrs['default']:
                try:
                    value = config_attrs['default']
                    m = re.match(r'^(\d+)e(\d+)$', value.lower())
                    if m:
                        _default = int(m.group(1)) * 10**int(m.group(2))
                    else:
                        _default = int(value, 0)
                except ValueError:
                    pass
        if _min is not None and _max is not None:
            if _min > _max:
                raise Exception('{} at {}:{} has min {} > max {}'.format(config_name, file_path, linenum, config_attrs['min'], config_attrs['max']))
        if _min is not None and _default is not None:
            if _min > _default:
                raise Exception('{} at {}:{} has min {} > default {}'.format(config_name, file_path, linenum, config_attrs['min'], config_attrs['default']))
        if _default is not None and _max is not None:
            if _default > _max:
                raise Exception('{} at {}:{} has default {} > max {}'.format(config_name, file_path, linenum, config_attrs['default'], config_attrs['max']))
    elif _type == 'bool':
        assert 'min' not in config_attrs
        assert 'max' not in config_attrs
        _default = config_attrs.get('default', None)
        if _default is not None:
            if '/' not in _default:
                if (_default not in ('0', '1')) and (_default not in all_config_names):
                    logger.info('{} at {}:{} has non-integer default value "{}"'.format(config_name, file_path, linenum, config_attrs['default']))

    elif _type == 'string':
        assert 'min' not in config_attrs
        assert 'max' not in config_attrs
        _default = config_attrs.get('default', None)
    elif _type == 'list':
        assert 'min' not in config_attrs
        assert 'max' not in config_attrs
        _default = config_attrs.get('default', None)
    else:
        raise Exception("Found unknown {} type {} at {}:{}".format(BASE_CMAKE_CONFIG_NAME, _type, file_path, linenum))




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
                        raise Exception("Found {} at {}:{} ({}) which isn't expected in {} files".format(BASE_CONFIG_NAME, file_path, linenum, line, filename if filename == 'CMakeLists.txt' else file_ext))
                    elif BASE_CMAKE_CONFIG_RE.search(line):
                        m = CMAKE_CONFIG_RE.match(line)
                        if not m:
                            if re.match("^\s*#\s*# ", line):
                                logger.info("Possible misformatted {} at {}:{} ({})".format(BASE_CMAKE_CONFIG_NAME, file_path, linenum, line))
                            else:
                                raise Exception("Found misformatted {} at {}:{} ({})".format(BASE_CMAKE_CONFIG_NAME, file_path, linenum, line))
                        else:
                            config_name = m.group(1)
                            config_description = m.group(2)
                            _attrs = m.group(3)
                            # allow commas to appear inside brackets by converting them to and from NULL chars
                            _attrs = re.sub(r'(\(.+\))', lambda m: m.group(1).replace(',', '\0'), _attrs)

                            if '=' in config_description:
                                raise Exception("For {} at {}:{} the description was set to '{}' - has the description field been omitted?".format(config_name, file_path, linenum, config_description))
                            all_descriptions = chips_all_descriptions[applicable]
                            if config_description in all_descriptions:
                                raise Exception("Found description {} at {}:{} but it was already used at {}:{}".format(config_description, file_path, linenum, os.path.join(scandir, all_descriptions[config_description]['filename']), all_descriptions[config_description]['line_number']))
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
                                    raise Exception('{} at {}:{} has malformed value {}'.format(config_name, file_path, linenum, item))
                                config_attrs[k] = v.replace('\0', ',')
                                all_attrs.add(k)
                                prev = item
                            #print(file_path, config_name, config_attrs)

                            if 'group' not in config_attrs:
                                raise Exception('{} at {}:{} has no group attribute'.format(config_name, file_path, linenum))

                            #print(file_path, config_name, config_attrs)
                            all_configs = chips_all_configs[applicable]
                            if config_name in all_configs:
                                raise Exception("Found {} at {}:{} but it was already declared at {}:{}".format(config_name, file_path, linenum, os.path.join(scandir, all_configs[config_name]['filename']), all_configs[config_name]['line_number']))
                            else:
                                all_configs[config_name] = {'attrs': config_attrs, 'filename': os.path.relpath(file_path, scandir), 'line_number': linenum, 'description': config_description}


all_config_names = set()
for all_configs in chips_all_configs.values():
    all_config_names.update(all_configs.keys())

for applicable, all_configs in chips_all_configs.items():
    for config_name, config_obj in all_configs.items():
        file_path = os.path.join(scandir, config_obj['filename'])
        linenum = config_obj['line_number']

        ValidateAttrs(config_name, config_obj['attrs'], file_path, linenum)

# All settings in "host" should also be in "all"
for config_name, config_obj in chips_all_configs["host"].items():
    if config_name not in chips_all_configs["all"]:
        file_path = os.path.join(scandir, config_obj['filename'])
        linenum = config_obj['line_number']
        raise Exception("Found 'host' config {} at {}:{}, but no matching non-host config found".format(config_name, file_path, linenum))

# Any chip-specific settings should not be in "all"
for chip in CHIP_NAMES:
    for config_name, chip_config_obj in chips_all_configs[chip].items():
        if config_name in chips_all_configs["all"]:
            all_config_obj = chips_all_configs["all"][config_name]
            chip_file_path = os.path.join(scandir, chip_config_obj['filename'])
            chip_linenum = chip_config_obj['line_number']
            all_file_path = os.path.join(scandir, all_config_obj['filename'])
            all_linenum = all_config_obj['line_number']
            raise Exception("'{}' config {} at {}:{} also found at {}:{}".format(chip, config_name, chip_file_path, chip_linenum, all_file_path, all_linenum))

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
                        raise Exception(build_mismatch_exception_message(config_name, field, applicable_config_obj, applicable_value, other_config_obj, other_value))
                # Check that attributes match
                for attr in applicable_config_obj['attrs']:
                    if attr != 'default': # totally fine for defaults to vary per-platform
                        applicable_value = applicable_config_obj['attrs'][attr]
                        other_value = other_config_obj['attrs'][attr]
                        if applicable_value != other_value:
                            raise Exception(build_mismatch_exception_message(config_name, "attribute '{}'".format(attr), applicable_config_obj, applicable_value, other_config_obj, other_value))

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
