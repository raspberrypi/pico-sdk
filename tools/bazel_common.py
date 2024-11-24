#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Common helpers and variables shared across Bazel-related Python scripts.

import argparse
import logging
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


_LOG = logging.getLogger(__file__)

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


def parse_common_args():
    parser = argparse.ArgumentParser()
    add_common_args(parser)
    return parser.parse_args()

def add_common_args(parser):
    parser.add_argument(
        "--picotool-dir",
        help="Use a local copy of Picotool rather than the dynamically fetching it",
        default=None,
        type=Path,
    )

def override_picotool_arg(picotool_dir):
    return f"--override_module=picotool={picotool_dir.resolve()}"


def bazel_command() -> str:
    """Return the path to bazelisk or bazel."""
    if shutil.which("bazelisk"):
        return shutil.which("bazelisk")
    if shutil.which("bazel"):
        return "bazel"

    raise FileNotFoundError(
        "Cannot find 'bazel' or 'bazelisk' in the current system PATH"
    )


def run_bazel(args, check=False, **kwargs):
    command = (
        bazel_command(),
        *args,
    )
    _LOG.info("Running Bazel command: %s", shlex.join(command))
    proc = subprocess.run(
        command,
        cwd=SDK_ROOT,
        **kwargs,
    )
    if proc.returncode != 0:
        _LOG.error("Command invocation failed with return code %d!", proc.returncode)
        _LOG.error(
            "Failing command: %s",
            " ".join(shlex.quote(str(arg)) for arg in args),
        )
        if kwargs.get("capture_output", False):
            output = (
                proc.stderr if isinstance(proc.stderr, str) else proc.stderr.decode()
            )
            _LOG.error(
                "Output:\n%s",
                output,
            )
        if check:
            raise subprocess.CalledProcessError(
                returncode=proc.returncode,
                cmd=command,
                output=proc.stdout,
                stderr=proc.stderr,
            )
    return proc


def print_to_stderr(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def print_framed_string(s):
    """Frames a string of text and prints it to highlight script steps."""
    header_spacer = "#" * (len(s) + 12)
    print_to_stderr(header_spacer)
    print_to_stderr("###   " + s + "   ###")
    print_to_stderr(header_spacer)


def setup_logging():
    log_levels = [
        (logging.ERROR, "\x1b[31m[ERROR]\x1b[0m"),
        (logging.WARNING, "\x1b[33m[WARNING]\x1b[0m"),
        (logging.INFO, "\x1b[35m[INFO]\x1b[0m"),
        (logging.DEBUG, "\x1b[34m[DEBUG]\x1b[0m"),
    ]
    for level, level_text in log_levels:
        logging.addLevelName(level, level_text)
    logging.basicConfig(format="%(levelname)s %(message)s", level=logging.DEBUG)
