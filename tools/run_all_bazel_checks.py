#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Runs all Bazel checks.

import argparse
import sys

from bazel_build import build_all_configurations
from bazel_common import (
    setup_logging,
    print_framed_string,
    print_to_stderr,
    add_common_args,
)
from compare_build_systems import compare_build_systems
from check_source_files_in_bazel_build import check_sources_in_bazel_build


def main():
    setup_logging()
    failed_steps = []

    parser = argparse.ArgumentParser()
    add_common_args(parser)
    parser.add_argument(
        "--program",
        help="A program to run",
        choices = [
            "all",
            "build",
            "other",
        ],
        default="all",
    )
    args = parser.parse_args()
    build_steps = (
        {
            "step_name": "build",
            "description": "Bazel build",
            "action": lambda : build_all_configurations(args.picotool_dir),
        },
    )
    other_steps = (
        {
            "description": "Ensure build system configurations options match",
            "action": compare_build_systems,
        },
        {
            "step_name": "check_srcs_in_build",
            "description": "Ensure source files are present in Bazel build",
            "action": lambda : check_sources_in_bazel_build(args.picotool_dir),
        },
    )
    steps_to_run = []
    run_all_steps = args.program == "all"
    if args.program == "build" or run_all_steps:
        steps_to_run.extend(build_steps)
    if args.program == "other" or run_all_steps:
        steps_to_run.extend(other_steps)

    for step in steps_to_run:
        print_framed_string(f"{step['description']}...")
        returncode = step["action"]()
        if returncode != 0:
            failed_steps.append(step["description"])
        print_to_stderr()

    if failed_steps:
        print_framed_string("ERROR: One or more steps failed.")
        for build in failed_steps:
            print_to_stderr(f"  * FAILED: {build}")
        return 1

    print_framed_string("All checks successfully passed!")


if __name__ == "__main__":
    sys.exit(main())
