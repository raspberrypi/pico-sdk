#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# A script to ensure that all declared configuration options match across both
# CMake and Bazel.
#
# Usage:
#
# Run from anywhere.

from dataclasses import dataclass
import glob
import os
import re
import subprocess
import sys

CMAKE_FILE_TYPES = (
    "**/CMakeLists.txt",
    "**/*.cmake",
)

BAZEL_FILE_TYPES = (
    "**/BUILD.bazel",
    "**/*.bzl",
    "**/*.BUILD",
)

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

SDK_ROOT = subprocess.run(
    (
        "git",
        "rev-parse",
        "--show-toplevel",
    ),
    cwd=SCRIPT_DIR,
    text=True,
    check=True,
    capture_output=True,
).stdout.strip()

ATTR_REGEX = re.compile(r',?\s*(?P<key>[^=]+)=(?P<value>[^,]+)')

# Sometimes the build systems are supposed to be implemented differently. This
# allowlist permits the descriptions to differ between CMake and Bazel.
BUILD_SYSTEM_DESCRIPTION_DIFFERENCE_ALLOWLIST = (
    "PICO_SDK_VERSION_MAJOR",
    "PICO_SDK_VERSION_MINOR",
    "PICO_SDK_VERSION_REVISION",
    "PICO_SDK_VERSION_PRE_RELEASE_ID",
    "PICO_SDK_VERSION_STRING",
    "PICO_DEFAULT_BOOT_STAGE2_FILE",
)

CMAKE_ONLY_ALLOWLIST = (
    # Not relevant to Bazel: toolchain is fetched dynamically, and can be
    # overridden with native Bazel features.
    "PICO_TOOLCHAIN_PATH",
    # TODO: No built-in, pre-configured clang offering yet.
    "PICO_COMPILER",
    # Entirely irrelevant to Bazel, use Bazel platforms:
    #     https://bazel.build/extending/platforms
    "PICO_CMAKE_PRELOAD_PLATFORM_FILE",
)

BAZEL_ONLY_ALLOWLIST = tuple()

@dataclass
class Option:
    name: str
    description: str
    attrs: dict[str, str]

    def matches(self, other):
        matches = (self.name == other.name) and (self.attrs == other.attrs)
        if not self.name in BUILD_SYSTEM_DESCRIPTION_DIFFERENCE_ALLOWLIST:
            matches = matches and (self.description == other.description)
        return matches


def FindKnownOptions(option_pattern_matcher, file_paths):
    pattern = re.compile(
        option_pattern_matcher +
        r':\s+(?P<name>\w+),\s+(?P<description>[^,]+)(?:,\s+(?P<attrs>.*))?$')
    options = {}
    for p in file_paths:
        with open(p, 'r') as f:
            for line in f:
                match = re.search(pattern, line)
                if not match:
                    continue

                attrs = {
                    m.group('key'): m.group('value')
                    for m in re.finditer(ATTR_REGEX, match.group('attrs'))
                }

                options[match.group('name')] = Option(
                    match.group('name'),
                    match.group('description'),
                    attrs,
                )
    return options


def OptionsAreEqual(bazel_option, cmake_option):
    if bazel_option is None:
        if cmake_option.name in CMAKE_ONLY_ALLOWLIST:
            return True
        print(f"    {cmake_option.name} does not exist in Bazel")
        return False
    elif cmake_option is None:
        if bazel_option.name in BAZEL_ONLY_ALLOWLIST:
            return True
        print(f"    {bazel_option.name} does not exist in CMake")
        return False
    elif not bazel_option.matches(cmake_option):
        print("    Bazel and CMAKE definitions do not match:")
        print(f"    [CMAKE]    {bazel_option}")
        print(f"    [BAZEL]    {cmake_option}")
        return False

    return True


def CompareOptions(bazel_pattern, bazel_files, cmake_pattern, cmake_files):
    bazel_options = FindKnownOptions(bazel_pattern, bazel_files)
    cmake_options = FindKnownOptions(cmake_pattern, cmake_files)

    are_equal = True
    both = {}
    both.update(bazel_options)
    both.update(cmake_options)
    for k in both.keys():
        if not OptionsAreEqual(bazel_options.get(k, None),
                               cmake_options.get(k, None)):
            are_equal = False
    return are_equal


cmake_files = [
    f for p in CMAKE_FILE_TYPES
    for f in glob.glob(p, root_dir=SDK_ROOT, recursive=True)
]
bazel_files = [
    f for p in BAZEL_FILE_TYPES
    for f in glob.glob(p, root_dir=SDK_ROOT, recursive=True)
]

print('[1/2] Checking build system configuration flags...')
build_options_ok = CompareOptions("PICO_BAZEL_CONFIG", bazel_files,
                                  "PICO_CMAKE_CONFIG", cmake_files)

print('[2/2] Checking build system defines...')
build_defines_ok = CompareOptions("PICO_BUILD_DEFINE", bazel_files,
                                  "PICO_BUILD_DEFINE", cmake_files)

if build_options_ok and build_defines_ok:
    print("OK")
    sys.exit(0)

sys.exit(1)