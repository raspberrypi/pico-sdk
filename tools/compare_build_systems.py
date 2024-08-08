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
import sys
from typing import Dict

from bazel_common import SDK_ROOT

CMAKE_FILE_TYPES = (
    "**/CMakeLists.txt",
    "**/*.cmake",
)

BAZEL_FILE_TYPES = (
    "**/BUILD.bazel",
    "**/*.bzl",
    "**/*.BUILD",
)

ATTR_REGEX = re.compile(r",?\s*(?P<key>[^=]+)=(?P<value>[^,]+)")

# Sometimes the build systems are supposed to be implemented differently. This
# allowlist permits the descriptions to differ between CMake and Bazel.
BUILD_SYSTEM_DESCRIPTION_DIFFERENCE_ALLOWLIST = (
    # Minor semantic differences in Bazel.
    "PICO_DEFAULT_BOOT_STAGE2_FILE",
    # In Bazel, not overridable by user environment variables (only flags).
    "PICO_BOARD",
    # In Bazel, it's a build label rather than a path.
    "PICO_CMSIS_PATH",
    # In Bazel, the semantics of embedded binary info are slightly different.
    "PICO_PROGRAM_NAME",
    "PICO_PROGRAM_DESCRIPTION",
    "PICO_PROGRAM_URL",
    "PICO_PROGRAM_VERSION_STRING",
    "PICO_TARGET_NAME",
)

CMAKE_ONLY_ALLOWLIST = (
    # Not relevant to Bazel: toolchain is fetched dynamically, and can be
    # overridden with native Bazel features.
    "PICO_TOOLCHAIN_PATH",
    # Bazel uses native --platforms mechanics.
    "PICO_PLATFORM",
    # TODO: No built-in, pre-configured clang offering for Bazel yet.
    "PICO_COMPILER",
    # Entirely irrelevant to Bazel, use Bazel platforms:
    #     https://bazel.build/extending/platforms
    "PICO_CMAKE_PRELOAD_PLATFORM_FILE",
    # Both of these are marked as TODO and not actually set up in CMake.
    "PICO_CMSIS_VENDOR",
    "PICO_CMSIS_DEVICE",
    # Bazel build uses PICO_CONFIG_EXTRA_HEADER and PICO_CONFIG_PLATFORM_HEADER
    # instead.
    "PICO_CONFIG_HEADER_FILES",
    "PICO_RP2040_CONFIG_HEADER_FILES",
    "PICO_HOST_CONFIG_HEADER_FILES",
    # Bazel uses PICO_CONFIG_HEADER.
    "PICO_BOARD_CMAKE_DIRS",
    "PICO_BOARD_HEADER_FILE",
    "PICO_BOARD_HEADER_DIRS",
    # Bazel supports this differently.
    # TODO: Provide a helper rule for explicitly generating a UF2 so users don't
    # have to write out a bespoke run_binary.
    "PICO_NO_UF2",
    # Bazel will not provide a default for this.
    # TODO: Provide handy rules for PIOASM so users don't have to write out a
    # bespoke run_binary.
    "PICO_DEFAULT_PIOASM_OUTPUT_FORMAT",
    # Bazel always has picotool.
    "PICO_NO_PICOTOOL",
    # TODO: Eventualy support.
    "PICO_NO_COPRO_DIS",
    "PICO_DEFAULT_RP2350_PLATFORM",
    "PICO_GCC_TRIPLE",
    "PICO_NO_FLASH",
    "PICO_COPY_TO_RAM",
    "PICO_RP2350_ARM_S_CONFIG_HEADER_FILES",
    "PICO_RP2350_RISCV_CONFIG_HEADER_FILES",
)

BAZEL_ONLY_ALLOWLIST = (
    # Allows users to fully replace the final image for boot_stage2.
    "PICO_BOOT_STAGE2_LINK_IMAGE",
    # Allows users to inject an alternative TinyUSB library since TinyUSB
    # doesn't have native Bazel support.
    "PICO_TINYUSB_LIB",
    # Bazel can't do pico_set_* for the binary info defines, so there's a
    # different mechanism.
    "PICO_DEFAULT_BINARY_INFO",
    # Bazel analogue for PICO_CMAKE_BUILD_TYPE.
    "PICO_BAZEL_BUILD_TYPE",
    # Different mechanism for setting a linker script that is less complex.
    "PICO_DEFAULT_LINKER_SCRIPT",
    # Not yet documented in CMake (but probably should be):
    "PICO_CMAKE_BUILD_TYPE",
    # Replaces PICO_RP2040_CONFIG_HEADER_FILES and
    # PICO_HOST_CONFIG_HEADER_FILES.
    "PICO_CONFIG_EXTRA_HEADER",
    "PICO_CONFIG_PLATFORM_HEADER",
    # Effectively replaces:
    # - PICO_BOARD_CMAKE_DIRS
    # - PICO_BOARD_HEADER_FILE
    # - PICO_BOARD_HEADER_DIRS
    "PICO_CONFIG_HEADER",
    # Bazel configuration for 3p deps.
    "PICO_BTSTACK_CONFIG",
    "PICO_LWIP_CONFIG",
    "PICO_FREERTOS_LIB",
    "PICO_MBEDTLS_LIB",
    # CMake has PICO_DEFAULT_CLIB, but it's not user-facing.
    "PICO_CLIB",
    # Selecting default library implementations.
    "PICO_MULTICORE_ENABLED",
    "PICO_DEFAULT_DOUBLE_IMPL",
    "PICO_DEFAULT_FLOAT_IMPL",
    "PICO_DEFAULT_DIVIDER_IMPL",
    "PICO_DEFAULT_PRINTF_IMPL",
    "PICO_DEFAULT_RAND_IMPL",
    "PICO_BINARY_INFO_ENABLED",
    # Allows selection of clang/gcc when using the dynamically fetched
    # toolchains.
    "PICO_TOOLCHAIN",
)


@dataclass
class Option:
    name: str
    description: str
    attrs: Dict[str, str]

    def matches(self, other):
        matches = (self.name == other.name) and (self.attrs == other.attrs)
        if not self.name in BUILD_SYSTEM_DESCRIPTION_DIFFERENCE_ALLOWLIST:
            matches = matches and (self.description == other.description)
        return matches


def FindKnownOptions(option_pattern_matcher, file_paths):
    pattern = re.compile(
        option_pattern_matcher
        + r":\s+(?P<name>\w+),\s+(?P<description>[^,]+)(?:,\s+(?P<attrs>.*))?$"
    )
    options = {}
    for p in file_paths:
        with open(p, "r") as f:
            for line in f:
                match = re.search(pattern, line)
                if not match:
                    continue

                attrs = {
                    m.group("key"): m.group("value")
                    for m in re.finditer(ATTR_REGEX, match.group("attrs"))
                }

                options[match.group("name")] = Option(
                    match.group("name"),
                    match.group("description"),
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
        if not OptionsAreEqual(bazel_options.get(k, None), cmake_options.get(k, None)):
            are_equal = False
    return are_equal


def compare_build_systems():
    cmake_files = [
        f
        for p in CMAKE_FILE_TYPES
        for f in glob.glob(os.path.join(SDK_ROOT, p), recursive=True)
    ]
    bazel_files = [
        f
        for p in BAZEL_FILE_TYPES
        for f in glob.glob(os.path.join(SDK_ROOT, p), recursive=True)
    ]

    print("[1/2] Checking build system configuration flags...")
    build_options_ok = CompareOptions(
        "PICO_BAZEL_CONFIG", bazel_files, "PICO_CMAKE_CONFIG", cmake_files
    )

    print("[2/2] Checking build system defines...")
    build_defines_ok = CompareOptions(
        "PICO_BUILD_DEFINE", bazel_files, "PICO_BUILD_DEFINE", cmake_files
    )

    if build_options_ok and build_defines_ok:
        print("OK")
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(compare_build_systems())
