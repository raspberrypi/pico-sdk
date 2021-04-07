#!/usr/bin/env python3
#
# Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Little script to build a header file including every other header file in the SDK!
# (also checks we don't have "conflicting" header-filenames)
# Edit the IGNORE_DIRS variable to filter out which directories get looked in.
#
# Usage:
#
# tools/build_all_headers.py <root of source tree> <output file>


import os
import sys

IGNORE_DIRS = set(['host', 'boards'])
IGNORE_DIRS.add('common/boot_picoboot')
IGNORE_DIRS.add('common/boot_uf2')
IGNORE_DIRS.add('rp2_common/pico_mem_ops')
IGNORE_DIRS.add('rp2_common/pico_stdio_semihosting')
IGNORE_DIRS.add('rp2_common/pico_stdio_usb')

if len(sys.argv) != 3:
    print("Usage: {} top_dir output_header".format(os.path.basename(sys.argv[0])))
    sys.exit(1)

top_dir = os.path.join(sys.argv[1], 'src')
output_header = sys.argv[2]

if not os.path.isdir(top_dir):
    print("{} doesn't exist!".format(top_dir))
    sys.exit(1)

include_dirs = set()
for root, dirs, files in os.walk(top_dir):
    prune_dirs = []
    for d in dirs:
        if os.path.relpath(os.path.join(root, d), top_dir) in IGNORE_DIRS:
            prune_dirs.append(d)
    for d in prune_dirs:
        dirs.remove(d)

    if 'include' in dirs:
        include_dirs.add(os.path.join(root, 'include'))
        dirs.remove('include')

include_files = list()
include_locations = dict()
for d in sorted(include_dirs):
    for root, dirs, files in os.walk(d):
        for f in sorted(files):
            if f.endswith('.h'):
                include_file = os.path.relpath(os.path.join(root, f), d)
                include_path = os.path.relpath(d, top_dir)
                if include_file in include_files:
                    raise Exception("Duplicate include file '{}' (found in both {} and {})".format(include_file, include_locations[include_file], include_path))
                include_files.append(include_file)
                include_locations[include_file] = include_path

with open(output_header, 'w') as fh:
    fh.write('''/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// This file is autogenerated, do not edit by hand

''')
    last_location = ''
    for f in include_files:
        if include_locations[f] != last_location:
            fh.write('\n// {}\n'.format(include_locations[f]))
        fh.write('#include "{}"\n'.format(f))
        last_location = include_locations[f]
    fh.write('\n')

