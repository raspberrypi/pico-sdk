#!/usr/bin/env python3
#
# Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for configuration items
# Outputs a tab separated file of the configuration item:
# name	location	platform	chip	description	type	advanced	default	depends	enumvalues	group	max	min
#
# Usage:
#
# tools/extract_configs.py <root of repo> [output file]
#
# If not specified, output file will be `pico_configs.tsv`


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
outfile = sys.argv[2] if len(sys.argv) > 2 else 'pico_configs.tsv'

BASE_CONFIG_NAME = 'PICO_CONFIG'
BASE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CONFIG_NAME))
BASE_CMAKE_CONFIG_NAME = 'PICO_CMAKE_CONFIG'
BASE_CMAKE_CONFIG_RE = re.compile(r'\b{}\b'.format(BASE_CMAKE_CONFIG_NAME))
BASE_BUILD_DEFINE_NAME = 'PICO_BUILD_DEFINE'
BASE_BUILD_DEFINE_RE = re.compile(r'\b{}\b'.format(BASE_BUILD_DEFINE_NAME))

CONFIG_RE = re.compile(r'//\s+{}:\s+(\w+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_CONFIG_NAME))
DEFINE_RE = re.compile(r'#define\s+(\w+)\s+(.+?)(\s*///.*)?$')

PROPERTY_TYPE = 'type'
PROPERTY_DEFAULT = 'default'
PROPERTY_MIN = 'min'
PROPERTY_MAX = 'max'
PROPERTY_ENUMVALUES = 'enumvalues'
PROPERTY_GROUP = 'group'
PROPERTY_ADVANCED = 'advanced'
PROPERTY_DEPENDS = 'depends'
ALLOWED_CONFIG_PROPERTIES = set([PROPERTY_TYPE, PROPERTY_DEFAULT, PROPERTY_MIN, PROPERTY_MAX, PROPERTY_ENUMVALUES, PROPERTY_GROUP, PROPERTY_ADVANCED, PROPERTY_DEPENDS])

PROPERTY_TYPE_INT = 'int'
PROPERTY_TYPE_BOOL = 'bool'
PROPERTY_TYPE_ENUM = 'enum'

CHIP_NAMES = ["rp2040", "rp2350"]

chips_all_configs = defaultdict(dict)
all_attrs = set()
chips_all_descriptions = defaultdict(dict)
chips_all_defines = defaultdict(dict)


def get_first_dict_key(some_dict):
    return list(some_dict.keys())[0]

def look_for_integer_define(config_name, attr_name, attr_str, file_path, linenum, applicable):
    defined_str = None
    if re.match('^\w+$', attr_str):
        # See if we have a matching define
        all_defines = chips_all_defines[applicable]
        if attr_str in all_defines:
            # There _may_ be multiple matching defines, but arbitrarily choose just one
            defined_str = get_first_dict_key(all_defines[attr_str])
        else:
            if applicable == 'all':
                # Look for it in the chip-specific defines
                found_chips = []
                missing_chips = []
                for chip in CHIP_NAMES:
                    all_defines = chips_all_defines[chip]
                    if attr_str in all_defines:
                        found_chips.append(chip)
                        # There _may_ be multiple matching defines, but arbitrarily choose just one
                        defined_str = get_first_dict_key(all_defines[attr_str])
                    else:
                        missing_chips.append(chip)
                # Report if it's found for some chips but not others (but ignore if it's missing for all)
                if found_chips and missing_chips:
                    logger.info('{} at {}:{} has {} value "{}" which has a matching define for {} but not for {}'.format(config_name, file_path, linenum, attr_name, attr_str, found_chips, missing_chips))
    return defined_str

def ValidateAttrs(config_name, config_attrs, file_path, linenum, applicable):
    type_str = config_attrs.get(PROPERTY_TYPE, PROPERTY_TYPE_INT)
    errors = []

    # Validate attrs
    for key in config_attrs.keys():
        if key not in ALLOWED_CONFIG_PROPERTIES:
            errors.append(Exception('{} at {}:{} has unexpected property "{}"'.format(config_name, file_path, linenum, key)))

    str_values = dict()
    parsed_values = dict()
    if type_str == PROPERTY_TYPE_INT:
        assert PROPERTY_ENUMVALUES not in config_attrs

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
                    defined_str = look_for_integer_define(config_name, attr_name, str_values[attr_name], file_path, linenum, applicable)
                    if defined_str is not None:
                        try:
                            m = re.match(r'^(\d+)e(\d+)$', str_values[attr_name].lower())
                            if m:
                                parsed_values[attr_name] = int(m.group(1)) * 10**int(m.group(2))
                            else:
                                parsed_values[attr_name] = int(defined_str, 0)
                        except ValueError:
                            logger.info('{} at {}:{} has {} value "{}" which resolves to the non-integer value "{}"'.format(config_name, file_path, linenum, attr_name, str_values[attr_name], defined_str))
                    else:
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
        assert PROPERTY_ENUMVALUES not in config_attrs

        attr_name = PROPERTY_DEFAULT
        str_values[attr_name] = config_attrs.get(attr_name, None)
        if str_values[attr_name] is not None:
            if (str_values[attr_name] not in ('0', '1')) and (str_values[attr_name] not in all_config_names):
                logger.info('{} at {}:{} has non-integer {} value "{}"'.format(config_name, file_path, linenum, attr_name, str_values[attr_name]))

    elif type_str == PROPERTY_TYPE_ENUM:
        assert PROPERTY_ENUMVALUES in config_attrs
        assert PROPERTY_MIN not in config_attrs
        assert PROPERTY_MAX not in config_attrs

        attr_name = PROPERTY_ENUMVALUES
        str_values[attr_name] = config_attrs[attr_name]
        parsed_values[attr_name] = tuple(str_values[attr_name].split('|'))

        attr_name = PROPERTY_DEFAULT
        str_values[attr_name] = config_attrs.get(attr_name, None)
        if str_values[attr_name] is not None:
            if str_values[attr_name] not in _enumvalues:
                errors.append(Exception('{} at {}:{} has {} value {} which isn\'t in list of {} {}'.format(config_name, file_path, linenum, attr_name, str_values[attr_name], PROPERTY_ENUMVALUES, str_values[PROPERTY_ENUMVALUES])))

    else:
        errors.append(Exception("Found unknown {} type {} at {}:{}".format(BASE_CONFIG_NAME, type_str, file_path, linenum)))

    return errors

errors = []

# Scan all .c and .h and .S files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    for filename in filenames:
        file_ext = os.path.splitext(filename)[1]
        if file_ext in ('.c', '.h', '.S'):
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
                    if BASE_CMAKE_CONFIG_RE.search(line):
                        errors.append(Exception("Found {} at {}:{} ({}) which isn't expected in {} files".format(BASE_CMAKE_CONFIG_NAME, file_path, linenum, line, file_ext)))
                    elif BASE_BUILD_DEFINE_RE.search(line):
                        errors.append(Exception("Found {} at {}:{} ({}) which isn't expected in {} files".format(BASE_BUILD_DEFINE_NAME, file_path, linenum, line, file_ext)))
                    elif BASE_CONFIG_RE.search(line):
                        m = CONFIG_RE.match(line)
                        if not m:
                            if re.match(r"^\s*//\s*// ", line):
                                logger.info("Possible misformatted {} at {}:{} ({})".format(BASE_CONFIG_NAME, file_path, linenum, line))
                            else:
                                errors.append(Exception("Found misformatted {} at {}:{} ({})".format(BASE_CONFIG_NAME, file_path, linenum, line)))
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
                    else:
                        m = DEFINE_RE.match(line)
                        if m:
                            name = m.group(1)
                            value = m.group(2)
                            # discard any 'u' qualifier
                            m = re.match(r'^((0x)?\d+)u$', value.lower())
                            if m:
                                value = m.group(1)
                            else:
                                # discard any '_u(X)' macro
                                m = re.match(r'^_u\(((0x)?\d+)\)$', value.lower())
                                if m:
                                    value = m.group(1)
                            all_defines = chips_all_defines[applicable]
                            if name not in all_defines:
                                all_defines[name] = dict()
                            if value not in all_defines[name]:
                                all_defines[name][value] = set()
                            all_defines[name][value] = (file_path, linenum)

all_config_names = set()
for all_configs in chips_all_configs.values():
    all_config_names.update(all_configs.keys())

# Check for defines with missing PICO_CONFIG entries
chips_resolved_defines = defaultdict(dict)
for applicable, all_defines in chips_all_defines.items():
    for d in all_defines:
        for define_prefix in ('PICO', 'PARAM', 'CYW43'):
            if d not in all_config_names and d.startswith(define_prefix+'_'):
                logger.warning("#define {} is potentially missing a {}: entry".format(d, BASE_CONFIG_NAME))
        resolved_defines = chips_resolved_defines[applicable]
        # resolve "nested defines" - this allows e.g. USB_DPRAM_MAX to resolve to USB_DPRAM_SIZE which is set to 4096 (which then matches the relevant PICO_CONFIG entry)
        for val in all_defines[d]:
            if val in all_defines:
                resolved_defines[d] = all_defines[val]

for applicable, all_configs in chips_all_configs.items():
    all_defines = chips_all_defines[applicable]
    resolved_defines = chips_resolved_defines[applicable]
    for config_name, config_obj in all_configs.items():
        file_path = os.path.join(scandir, config_obj['filename'])
        linenum = config_obj['line_number']

        errors.extend(ValidateAttrs(config_name, config_obj['attrs'], file_path, linenum, applicable))

        # Check that default values match up
        if 'default' in config_obj['attrs']:
            config_default = config_obj['attrs']['default']
            if config_name in all_defines:
                defines_obj = all_defines[config_name]
                if config_default not in defines_obj and (config_name not in resolved_defines or config_default not in resolved_defines[config_name]):
                    if '/' in config_default or ' ' in config_default:
                        continue
                    # There _may_ be multiple matching defines, but arbitrarily display just one in the error message
                    first_define_value = get_first_dict_key(defines_obj)
                    first_define_file_path, first_define_linenum = defines_obj[first_define_value]
                    errors.append(Exception('Found {} at {}:{} with a default of {}, but #define says {} (at {}:{})'.format(config_name, file_path, linenum, config_default, first_define_value, first_define_file_path, first_define_linenum)))
            else:
                errors.append(Exception('Found {} at {}:{} with a default of {}, but no matching #define found'.format(config_name, file_path, linenum, config_default)))

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
