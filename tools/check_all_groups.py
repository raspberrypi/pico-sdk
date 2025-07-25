#!/usr/bin/env python3
#
# Copyright (c) 2025 Raspberry Pi Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to check that the "groups" in various contexts all match up. (like an enhanced version of check_doxygen_groups.py)
# Note that it only reports the *first* instance of a missing group, not all occurrences.
#
# Usage:
#
# tools/check_all_groups.py <root of repo>
#   (you'll probably want to pipe the output through ` | grep -v cmsis` )

import re
import sys
import os

scandir = sys.argv[1]

DEFGROUP_NAME = r'\defgroup'
DEFGROUP_RE = re.compile(r'{}\s+(\w+)'.format(re.escape(DEFGROUP_NAME)))
INGROUP_NAME = r'\ingroup'
INGROUP_RE = re.compile(r'{}\s+(\w+)'.format(re.escape(INGROUP_NAME)))
DOCS_INDEX_HEADER = 'docs/index.h'

BASE_CONFIG_NAME = 'PICO_CONFIG'
CONFIG_RE = re.compile(r'//\s+{}:\s+(\w+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_CONFIG_NAME))
BASE_CMAKE_CONFIG_NAME = 'PICO_CMAKE_CONFIG'
CMAKE_CONFIG_RE = re.compile(r'#\s+{}:\s+([\w-]+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_CMAKE_CONFIG_NAME))
BASE_BUILD_DEFINE_NAME = 'PICO_BUILD_DEFINE'
BUILD_DEFINE_RE = re.compile(r'#\s+{}:\s+(\w+),\s+([^,]+)(?:,\s+(.*))?$'.format(BASE_BUILD_DEFINE_NAME))
BASE_GROUP_NAME = 'group='
GROUP_RE = re.compile(r'\b{}(\w+)\b'.format(BASE_GROUP_NAME))

def_groups = {}
in_groups = {}
doc_groups = {}
config_groups = {}
cmake_config_groups = {}
build_define_groups = {}
any_errors = False

def get_group_from_config_attrs(attr_str):
    m = GROUP_RE.search(attr_str)
    if m:
        return m.group(1)

# Scan all .c and .h and .S and .cmake and CMakeLists.txt files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    for filename in filenames:
        file_ext = os.path.splitext(filename)[1]
        if filename == 'CMakeLists.txt' or file_ext in ('.c', '.h', '.S', '.cmake'):
            file_path = os.path.join(dirpath, filename)
            with open(file_path) as fh:
                for line in fh.readlines():
                    m = DEFGROUP_RE.search(line)
                    if m:
                        group = m.group(1)
                        if file_path.endswith(DOCS_INDEX_HEADER):
                            if group in doc_groups:
                                any_errors = True
                                print("{} uses {} {} but so does {}".format(doc_groups[group], DEFGROUP_NAME, group, file_path))
                            else:
                                doc_groups[group] = file_path
                        else:
                            if group in def_groups:
                                any_errors = True
                                print("{} uses {} {} but so does {}".format(def_groups[group], DEFGROUP_NAME, group, file_path))
                            else:
                                def_groups[group] = file_path
                    else:
                        m = INGROUP_RE.search(line)
                        if m:
                            group = m.group(1)
                            if group not in in_groups:
                                in_groups[group] = file_path
                        else:
                            m = CONFIG_RE.search(line)
                            if m:
                                group = get_group_from_config_attrs(m.group(3))
                                if group not in config_groups:
                                    config_groups[group] = file_path
                            else:
                                m = CMAKE_CONFIG_RE.search(line)
                                if m:
                                    group = get_group_from_config_attrs(m.group(3))
                                    if group not in cmake_config_groups:
                                        cmake_config_groups[group] = file_path
                                else:
                                    m = BUILD_DEFINE_RE.search(line)
                                    if m:
                                        group = get_group_from_config_attrs(m.group(3))
                                        if group not in build_define_groups:
                                            build_define_groups[group] = file_path

seen_groups = set()
for group, file_path in in_groups.items():
    seen_groups.add(group)
    if group not in def_groups and group not in doc_groups:
        any_errors = True
        print("{} uses {} {} which was never defined".format(file_path, INGROUP_NAME, group))
for group, file_path in config_groups.items():
    seen_groups.add(group)
    if group not in def_groups and group not in doc_groups:
        any_errors = True
        print("{} uses {} {}{} which was never defined".format(file_path, BASE_CONFIG_NAME, BASE_GROUP_NAME, group))
for group, file_path in cmake_config_groups.items():
    seen_groups.add(group)
    if group == 'build': # dummy group
        continue
    if group not in def_groups and group not in doc_groups:
        any_errors = True
        print("{} uses {} {}{} which was never defined".format(file_path, BASE_CMAKE_CONFIG_NAME, BASE_GROUP_NAME, group))
for group, file_path in build_define_groups.items():
    seen_groups.add(group)
    if group == 'build': # dummy group
        continue
    if group not in def_groups and group not in doc_groups:
        any_errors = True
        print("{} uses {} {}{} which was never defined".format(file_path, BASE_BUILD_DEFINE_NAME, BASE_GROUP_NAME, group))

for group in doc_groups.keys():
    seen_groups.add(group)
    if group not in seen_groups and group not in def_groups:
        any_errors = True
        print("{} uses {} {} which doesn't appear anywhere else".format(DOCS_INDEX_HEADER, DEFGROUP_NAME, group))

unused_groups = set(def_groups.keys()) - seen_groups
if unused_groups:
    any_errors = True
    print("The following groups were defined with {} but never referenced:\n{}".format(DEFGROUP_NAME, sorted(unused_groups)))

sys.exit(any_errors)
