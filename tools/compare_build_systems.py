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
import logging
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Dict

from bazel_common import SDK_ROOT, setup_logging

_LOG = logging.getLogger(__file__)

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

BAZEL_MODULE_REGEX = re.compile(r'\s*commit\s*=\s*\"(?P<commit>[0-9a-fA-F]+)\"\s*,\s*#\s*keep-in-sync-with-submodule:\s*(?P<dependency>\S*)')

BAZEL_VERSION_REGEX = re.compile(r'module\(\s*name\s*=\s*"pico-sdk",\s*version\s*=\s*"(?P<sdk_version>[^"]+)",?\s*\)')

CMAKE_VERSION_REGEX = re.compile(r'^[^#]*set\(PICO_SDK_VERSION_(?P<part>\S+)\s+(?P<value>\S+)\)')

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
    # Named PICO_TOOLCHAIN in Bazel.
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
    # These aren't supported as build flags in Bazel. Prefer to
    # set these in board header files like other SDK defines.
    "CYW43_DEFAULT_PIN_WL_REG_ON",
    "CYW43_DEFAULT_PIN_WL_DATA_OUT",
    "CYW43_DEFAULT_PIN_WL_DATA_IN",
    "CYW43_DEFAULT_PIN_WL_HOST_WAKE",
    "CYW43_DEFAULT_PIN_WL_CLOCK",
    "CYW43_DEFAULT_PIN_WL_CS",
    "CYW43_PIO_CLOCK_DIV_INT",
    "CYW43_PIO_CLOCK_DIV_FRAC",
    "CYW43_PIO_CLOCK_DIV_DYNAMIC",
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
    "PICO_ASYNC_CONTEXT_IMPL",
    # Allows selection of clang/gcc when using the dynamically fetched
    # toolchains.
    "PICO_TOOLCHAIN",
    # In CMake, linking these libraries also sets defines for adjacent
    # libraries. That's an antipattern in Bazel, so there's flags to control
    # which modules to enable instead.
    "PICO_BT_ENABLE_BLE",
    "PICO_BT_ENABLE_CLASSIC",
    "PICO_BT_ENABLE_MESH",
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


def OptionsAreEqual(bazel_option, cmake_option, warnings_as_errors):
    if bazel_option is None:
        if cmake_option.name in CMAKE_ONLY_ALLOWLIST:
            return True
        _LOG.warning(f"    {cmake_option.name} does not exist in Bazel")
        return not warnings_as_errors
    elif cmake_option is None:
        if bazel_option.name in BAZEL_ONLY_ALLOWLIST:
            return True
        _LOG.warning(f"    {bazel_option.name} does not exist in CMake")
        return not warnings_as_errors
    elif not bazel_option.matches(cmake_option):
        _LOG.error("    Bazel and CMAKE definitions do not match:")
        _LOG.error(f"    [CMAKE]    {bazel_option}")
        _LOG.error(f"    [BAZEL]    {cmake_option}")
        return False

    return True


def CompareOptions(bazel_pattern, bazel_files, cmake_pattern, cmake_files, warnings_as_errors=True):
    bazel_options = FindKnownOptions(bazel_pattern, bazel_files)
    cmake_options = FindKnownOptions(cmake_pattern, cmake_files)

    are_equal = True
    both = {}
    both.update(bazel_options)
    both.update(cmake_options)
    for k in both.keys():
        if not OptionsAreEqual(
            bazel_options.get(k, None),
            cmake_options.get(k, None),
            warnings_as_errors,
        ):
            are_equal = False
    return are_equal

def CompareExternalDependencyVersions():
    pattern = re.compile(BAZEL_MODULE_REGEX)
    all_okay = True
    with open(Path(SDK_ROOT) / "MODULE.bazel", "r") as bazel_module_file:
        for line in bazel_module_file:
            maybe_match = pattern.match(line)
            if not maybe_match:
                continue

            current_submodule_pin = subprocess.run(
                ("git", "-C", SDK_ROOT, "rev-parse", f'HEAD:{maybe_match.group("dependency")}'),
                text=True,
                check=True,
                capture_output=True,
            ).stdout.strip()
            if current_submodule_pin != maybe_match.group("commit"):
                _LOG.error("    External pins for %s do not match:", maybe_match.group("dependency"))
                _LOG.error("    [CMAKE]    %s", current_submodule_pin)
                _LOG.error("    [BAZEL]    %s", maybe_match.group("commit"))
                all_okay = False
            else:
                _LOG.info("    External pins for %s match!", maybe_match.group("dependency"))

    return all_okay

def CompareSdkVersion():
    # Find version string specified in Bazel.
    bazel_module_file_path = Path(SDK_ROOT) / "MODULE.bazel"
    bazel_module_file_contents = bazel_module_file_path.read_text()
    bazel_sdk_version = BAZEL_VERSION_REGEX.search(bazel_module_file_contents)
    if not bazel_sdk_version:
        _LOG.error("    Failed to find Bazel Pico SDK version string")
        return False
    bazel_version_string = bazel_sdk_version.group("sdk_version")

    # Find version string specified in CMake.
    cmake_version_parts = {}
    with open(Path(SDK_ROOT) / "pico_sdk_version.cmake", "r") as cmake_version_file:
        for line in cmake_version_file:
            match = CMAKE_VERSION_REGEX.match(line)
            if match:
                cmake_version_parts[match.group("part")] = match.group("value")
    if len(cmake_version_parts) < 3:
        _LOG.error("    Failed to find CMake Pico SDK version string")
        return False
    cmake_version_string = ".".join((
        cmake_version_parts["MAJOR"],
        cmake_version_parts["MINOR"],
        cmake_version_parts["REVISION"],
    ))
    if "PRE_RELEASE_ID" in cmake_version_parts:
        cmake_version_string += "-" + cmake_version_parts["PRE_RELEASE_ID"]

    if cmake_version_string != bazel_version_string:
        _LOG.error("    Declared CMake SDK version is %s and Bazel is %s", cmake_version_string, bazel_version_string)
        return False

    return True

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

    results = []
    _LOG.info("[1/3] Checking build system configuration flags...")
    results.append(CompareOptions(
        "PICO_BAZEL_CONFIG",
        bazel_files,
        "PICO_CMAKE_CONFIG",
        cmake_files,
        # For now, allow CMake and Bazel to go out of sync when it comes to
        # build configurability since it's a big ask to make contributors
        # implement the same functionality in both builds.
        warnings_as_errors=False,
    ))

    _LOG.info("[2/4] Checking build system defines...")
    results.append(CompareOptions(
        "PICO_BUILD_DEFINE", bazel_files, "PICO_BUILD_DEFINE", cmake_files
    ))

    _LOG.info("[3/4] Checking submodule pins...")
    results.append(CompareExternalDependencyVersions())

    _LOG.info("[4/4] Checking version strings...")
    results.append(CompareSdkVersion())

    if False not in results:
        _LOG.info("Passed with no blocking failures")
        return 0

    _LOG.error("One or more blocking failures detected")
    return 1


if __name__ == "__main__":
    setup_logging()
    sys.exit(compare_build_systems())
