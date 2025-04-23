#!/usr/bin/env python3
#
# Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for CMake functions
# Outputs a tab separated file of the function:
# name	group	signature	brief	description	params
#
# Usage:
#
# tools/extract_cmake_functions.py <root of repo> [output file]
#
# If not specified, output file will be `pico_cmake_functions.tsv`


import os
import sys
import re
import csv
import logging

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

scandir = sys.argv[1]
outfile = sys.argv[2] if len(sys.argv) > 2 else 'pico_cmake_functions.tsv'

CMAKE_FUNCTION_RE = re.compile(r'^\s*#(.*)((\n\s*#.*)*)\n\s*function\(([^\s]*)', re.MULTILINE)

CMAKE_PICO_FUNCTIONS_RE = re.compile(r'^\s*function\((pico_[^\s\)]*)', re.MULTILINE)

# Files containing internal functions that don't need to be documented publicly
skip_files = set([
    "pico_sdk_init.cmake",
    "pico_utils.cmake",
    "no_hardware.cmake",
    "find_compiler.cmake",
])

skip_groups = set([
    "src",  # skip the root src/CMakeLists.txt
])

# Other internal functions that don't need to be documented publicly
allowed_missing_functions = set([
    "pico_init_pioasm",
    "pico_init_picotool",
    "pico_add_platform_library",
    "pico_get_runtime_output_directory",
    "pico_set_printf_implementation",
    "pico_expand_pico_platform",
])

all_functions = {}

# Supported commands:
# \brief\ <brief description, which should be included in the main description>
# \brief_nodesc\ <brief description, which should be excluded from the main description>
# \param\ <parameter_name> <parameter description>
#
# Commands in the middle of a line are not supported
#
# The ; character at the end of a line denotes a hard line break in the description
# The \ character (outside of a command) and the # character are not supported in descriptions
def process_commands(description, name, group, signature):
    brief = ''
    params = []
    desc = ''
    for line in description.split('\n'):
        line = line.strip()
        if line.startswith('\\'):
            command = line.split('\\')[1]
            if command == 'param':
                # Parameter name and description
                params.append(line.split('\\')[2].strip())
            elif command == 'brief':
                # Brief description
                brief = line.split('\\')[2].strip()
                desc += brief + '\\n'
            elif command == 'brief_nodesc':
                # Brief description which should not be included in the main description
                brief = line.split('\\')[2].strip()
            else:
                logger.error("{}:{} has unknown command: {}".format(group, name, command))
        elif '\\' in line:
            logger.error("{}:{} has a line containing '\\': {}".format(group, name, line))
        else:
            desc += line + '\\n'
    # Check that there are no semicolons in the parameter descriptions, as that's the delimiter for the parameter list
    if any([';' in x for x in params]):
        logger.error("{}:{} has a parameter description containing ';'".format(group, name))
    # Check that all parameters are in the signature
    for param in params:
        if param.split(' ')[0] not in signature:
            logger.error("{}:{} has a parameter {} which is not in the signature {}".format(group, name, param.split(' ')[0], signature))
    # Check that the brief description is not empty
    if not brief:
        logger.warning("{}:{} has no brief description".format(group, name))

    return desc.strip(), brief, ';'.join(params)


def sort_functions(item):
    group = item[1]['group']
    name = item[1]['name']

    precedence = 5
    if group == 'other':
        if name == 'pico_generate_pio_header':
            precedence = 0
        elif re.match(r'^pico_add_.*_output$', name):
            precedence = 1
        elif name == 'pico_add_extra_outputs':
            precedence = 2
        elif re.match(r'^pico_.*_binary$', name):
            precedence = 3
    return group + str(precedence) + name


# Scan all CMakeLists.txt and .cmake files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    for filename in filenames:
        if filename in skip_files:
            continue
        group = os.path.basename(dirpath)
        if group in skip_groups:
            continue
        if group in ['tools', 'cmake']:
            group = 'other'
        file_ext = os.path.splitext(filename)[1]
        if filename == 'CMakeLists.txt' or file_ext == '.cmake':
            file_path = os.path.join(dirpath, filename)

            with open(file_path, encoding="ISO-8859-1") as fh:
                text = fh.read()
                for match in CMAKE_FUNCTION_RE.finditer(text):
                    name = match.group(4)
                    signature = match.group(1).strip()
                    if signature.startswith(name):
                        description, brief, params = process_commands(match.group(2).replace('#', ''), name, group, signature)
                        all_functions[name] = {
                            'name': name,
                            'group': group,
                            'signature': signature,
                            'brief': brief,
                            'description': description,
                            'params': params
                        }

                for match in CMAKE_PICO_FUNCTIONS_RE.finditer(text):
                    name = match.group(1)
                    if name not in all_functions and name not in allowed_missing_functions:
                        logger.warning("{} function has no description in {}".format(name, file_path))


with open(outfile, 'w', newline='') as csvfile:
    fieldnames = ('name', 'group', 'signature', 'brief', 'description', 'params')
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore', dialect='excel-tab')

    writer.writeheader()
    for name, row in sorted(all_functions.items(), key=sort_functions):
        writer.writerow(row)
