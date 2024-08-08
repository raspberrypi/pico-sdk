#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Runs all Bazel checks.

import sys

from bazel_build import build_all_configurations
from bazel_common import setup_logging, print_framed_string, parse_common_args
from compare_build_systems import compare_build_systems
from check_source_files_in_bazel_build import check_sources_in_bazel_build


def main():
    setup_logging()
    failed_steps = []

    args = parse_common_args()

    steps = (
        {
            "description": "Bazel build",
            "action": lambda : build_all_configurations(args.picotool_dir),
        },
        {
            "description": "Ensure build system configurations options match",
            "action": compare_build_systems,
        },
        {
            "description": "Ensure source files are present in Bazel build",
            "action": lambda : check_sources_in_bazel_build(args.picotool_dir),
        },
    )

    for step in steps:
        print_framed_string(f"{step['description']}...")
        returncode = step["action"]()
        if returncode != 0:
            failed_steps.append(step["description"])
        print()

    if failed_steps:
        print_framed_string("ERROR: One or more steps failed.")
        for build in failed_steps:
            print(f"  * FAILED: {build}")
        return 1

    print_framed_string("All checks successfully passed!")


if __name__ == "__main__":
    sys.exit(main())
