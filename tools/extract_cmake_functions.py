#!/usr/bin/env python3
#
# Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for CMake functions
# Outputs a tab separated file of the functions:
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

if sys.version_info < (3, 11):
    # Python <3.11 doesn't have ExceptionGroup, so define a simple one
    class ExceptionGroup(Exception):
        def __init__(self, message, errors):
            message += "\n" + "\n".join(str(e) for e in errors)
            super().__init__(message)

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
    "pico_get_output_name",
    "pico_set_printf_implementation",
    "pico_expand_pico_platform",
])

# Group descriptions
group_names_descriptions = {
    'boot_stage2': ('Boot Stage 2', 'CMake functions to create stage 2 bootloaders'),
    'pico_binary_info': ('Pico Binary Info', 'CMake functions to add binary info to the output binary'),
    'pico_btstack': ('Pico BTstack', 'CMake functions to configure the bluetooth stack'),
    'pico_lwip': ('Pico LwIP', 'CMake functions to configure LwIP'),
    'pico_cyw43_driver': ('Pico CYW43 Driver', 'CMake functions to configure the CYW43 driver'),
    'pico_runtime': ('Pico Runtime', 'CMake functions to configure the runtime environment'),
    'pico_standard_link': ('Pico Standard Link', 'CMake functions to configure the linker'),
    'pico_stdio': ('Pico Standard I/O', 'CMake functions to configure the standard I/O library'),
    'pico_pio': ('Pico PIO', 'CMake functions to generate PIO headers'),
    'other': ('Other', 'Other CMake functions'),
}


all_functions = {}

for group, (brief, description) in group_names_descriptions.items():
    all_functions['_desc_{group}'.format(group=group)] = {
        'name': '_desc_{group}'.format(group=group),
        'group': group,
        'signature': '',
        'brief': brief,
        'description': description,
        'params': '',
    }

# Supported commands:
# \brief\ <brief description, which should be included in the main description>
# \brief_nodesc\ <brief description, which should be excluded from the main description>
# \param\ <parameter_name> <parameter description>
# \ingroup\ <group_name>
#
# Commands in the middle of a line are not supported
#
# The ; character at the end of a line denotes a hard line break in the description
# The \ character (outside of a command) and the # character are not supported in descriptions
def process_commands(description, name, group, signature):
    brief = ''
    params = []
    desc = ''
    errors = []
    for line in description.split('\n'):
        line = line.strip()
        if line.startswith('\\'):
            _, command, remainder = line.split('\\', 2)
            remainder = remainder.strip()
            if command == 'param':
                # Parameter name and description
                params.append(remainder)
            elif command == 'brief':
                # Brief description
                brief = remainder
                desc += brief + '\\n'
            elif command == 'brief_nodesc':
                # Brief description which should not be included in the main description
                brief = remainder
            elif command == 'ingroup':
                # Group name override
                group = remainder
            else:
                errors.append(Exception("{}:{} has unknown command: {}".format(group, name, command)))
        elif '\\' in line:
            errors.append(Exception("{}:{} has a line containing '\\': {}".format(group, name, line)))
        else:
            desc += line + '\\n'
    # Check that there are no semicolons in the parameter descriptions, as that's the delimiter for the parameter list
    if any([';' in x for x in params]):
        errors.append(Exception("{}:{} has a parameter description containing ';'".format(group, name)))
    # Check that all parameters are in the signature
    signature_words = set(re.split(r'\W+', signature))
    for param in params:
        param_name = param.split(' ', maxsplit=1)[0]
        if param_name not in signature_words:
            errors.append(Exception("{}:{} has a parameter {} which is not in the signature {}".format(group, name, param_name, signature)))
    # Check that the brief description is not empty
    if not brief:
        logger.warning("{}:{} has no brief description".format(group, name))
    # Check that the group has a description
    if group not in group_names_descriptions:
        errors.append(Exception("{} has no group description (referenced from {})".format(group, name)))

    desc = re.sub(r'^(\\n)*(.*?)(\\n)*$', r'\2', desc)
    return desc.strip(), brief, ';'.join(params), group, errors


def sort_functions(item):
    group = item['group']
    name = item['name']

    precedence = 5
    if name.startswith('_desc_'):
        precedence = 0
    elif group == 'other':
        if re.match(r'^pico_add_.*_output$', name):
            precedence = 1
        elif name == 'pico_add_extra_outputs':
            precedence = 2
        elif re.match(r'^pico_.*_binary$', name):
            precedence = 3
    return group + str(precedence) + name

all_errors = []

# Scan all CMakeLists.txt and .cmake files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    for filename in filenames:
        if filename in skip_files:
            continue
        # Default group is the directory name - can be overridden by the \ingroup\ command
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
                        description, brief, params, processed_group, errors = process_commands(match.group(2).replace('#', ''), name, group, signature)
                        all_errors.extend(errors)
                        new_dict = {
                            'name': name,
                            'group': processed_group,
                            'signature': signature,
                            'brief': brief,
                            'description': description,
                            'params': params
                        }
                        if all_functions.get(name):
                            if new_dict != all_functions[name]:
                                logger.warning("{}:{} has multiple different definitions - using the new one from {}".format(processed_group, name, file_path))
                        all_functions[name] = new_dict

                for match in CMAKE_PICO_FUNCTIONS_RE.finditer(text):
                    name = match.group(1)
                    if name not in all_functions and name not in allowed_missing_functions:
                        logger.warning("{} function has no description in {}".format(name, file_path))

if all_errors:
    raise ExceptionGroup("Errors in {}".format(outfile), all_errors)


with open(outfile, 'w', newline='') as csvfile:
    fieldnames = ('name', 'group', 'signature', 'brief', 'description', 'params')
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore', dialect='excel-tab')

    writer.writeheader()
    for row in sorted(all_functions.values(), key=sort_functions):
        writer.writerow(row)
