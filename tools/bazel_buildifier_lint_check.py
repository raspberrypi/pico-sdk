#!/usr/bin/env python3
#
# Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Run buildifier check on all Bazel files.
#

import logging
import os
from pathlib import Path
from typing import Iterable
import subprocess
import sys

from bazel_common import (
    SDK_ROOT,
    run_bazel,
    setup_logging,
    print_framed_string,
    print_to_stderr,
)

_LOG = logging.getLogger(__file__)


def check_buildifier() -> int:
    """Runs the buildifier check on all the pico-sdk Bazel build files.

    This catches ensures consistent formatting and documentation, and catches
    lint errors.
    """

    # Get all files from git that we'd like to lint.
    patterns = ["*.bazel", "*.bzl", "WORKSPACE", "*.BUILD"]
    git_command = ["git", "ls-files"] + patterns
    result = subprocess.run(
        git_command,
        cwd=SDK_ROOT,
        text=True,
        check=True,
        capture_output=True,
    ).stdout

    # Use absolute paths to ensure this works from any directory.
    build_files = [
        os.path.abspath(os.path.join(SDK_ROOT, f)) for f in result.splitlines()
    ]
    if not build_files:
        _LOG.info("No files to check.")
        return 0

    _LOG.info(f"Running buildifier on {len(build_files)} targets...")

    args = [
        "run",
        "@buildifier_prebuilt//:buildifier",
        "--",
        "--mode=check",
        "--lint=warn",
    ] + build_files

    proc = run_bazel(args, check=False, capture_output=True, text=True)

    if proc.returncode != 0:
        _LOG.error("ERROR: One or more buildifier checks failed.")
        print_to_stderr("\nTo automatically fix formatting issues in a file, run:")
        print_to_stderr("  bazel run @buildifier_prebuilt//:buildifier -- <file>")
        print_to_stderr("\nTo run the checks manually on a file, run:")
        print_to_stderr(
            "  bazel run @buildifier_prebuilt//:buildifier -- --mode=check --lint=warn <file>\n"
        )
        return proc.returncode

    _LOG.info("\x1b[32mBuildifier checks passed.\x1b[0m")
    return 0


if __name__ == "__main__":
    setup_logging()
    sys.exit(check_buildifier())
