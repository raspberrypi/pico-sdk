#!/usr/bin/env python3
#
# Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Script to scan the Raspberry Pi Pico SDK tree searching for CMake functions
# Outputs a tab separated file of the function:
# name	signature	description
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

from collections import defaultdict

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

scandir = sys.argv[1]
outfile = sys.argv[2] if len(sys.argv) > 2 else 'pico_cmake_functions.tsv'

CMAKE_FUNCTION_RE = re.compile(r'^#(.*)((\n\s*#.*)*)\nfunction\(([^\s]*)', re.MULTILINE)


all_functions = {}


# Scan all CMakeLists.txt and .cmake files in the specific path, recursively.

for dirpath, dirnames, filenames in os.walk(scandir):
    # dirnames[:] = [d for d in dirnames if d != "lib"]
    for filename in filenames:
        file_ext = os.path.splitext(filename)[1]
        if filename == 'CMakeLists.txt' or file_ext == '.cmake':
            file_path = os.path.join(dirpath, filename)

            with open(file_path, encoding="ISO-8859-1") as fh:
                text = fh.read()
                for match in CMAKE_FUNCTION_RE.finditer(text):
                    name = match.group(4)
                    signature = match.group(1).strip()
                    description = match.group(2)
                    description = description.replace('\n#\n', '\n\n').strip()
                    description = description.replace('\n#', '').strip()
                    description = description.replace('#', '').strip()
                    if signature.startswith(name):
                        all_functions[name] = (signature, description)


print(all_functions)


with open(outfile, 'w', newline='') as csvfile:
    fieldnames = ('name', 'signature', 'description')
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore', dialect='excel-tab')

    writer.writeheader()
    for name, (signature, description) in sorted(all_functions.items()):
        writer.writerow({'name': name, 'signature': signature, 'description': description})
