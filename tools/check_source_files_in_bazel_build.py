#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Check Bazel build file source coverage. Reports files that:
# - Are in the repo but not included in a BUILD.bazel file.
# - Are referenced in a BUILD.bazel file but are not present.
#
# Usage:
#   python tools/check_source_files_in_bazel_build.py
#
# Run from anywhere in the pico-sdk repo.

import logging
from pathlib import Path
import shlex
import subprocess
from typing import (
    Container,
    Iterable,
    List,
    Optional,
    Set,
)
import sys

from bazel_common import (
    SDK_ROOT,
    bazel_command,
    override_picotool_arg,
    parse_common_args,
    setup_logging,
)

_LOG = logging.getLogger(__file__)

CPP_HEADER_EXTENSIONS = (
    ".h",
    ".hpp",
    ".hxx",
    ".h++",
    ".hh",
    ".H",
)
CPP_SOURCE_EXTENSIONS = (
    ".c",
    ".cpp",
    ".cxx",
    ".c++",
    ".cc",
    ".C",
    ".S",
    ".inc",
    ".inl",
)

IGNORED_FILE_PATTERNS = (
    # Doxygen only files.
    "**/index.h",
    "**/doc.h",
)


def get_paths_from_command(source_dir: Path, *args, **kwargs) -> Set[Path]:
    """Runs a command and reads Bazel //-style paths from it."""
    process = subprocess.run(
        args, check=False, capture_output=True, cwd=source_dir, **kwargs
    )

    if process.returncode:
        _LOG.error("Command invocation failed with return code %d!", process.returncode)
        _LOG.error(
            "Command: %s",
            " ".join(shlex.quote(str(arg)) for arg in args),
        )
        _LOG.error(
            "Output:\n%s",
            process.stderr.decode(),
        )
        sys.exit(1)

    files = set()

    for line in process.stdout.splitlines():
        path = line.strip().lstrip(b"/").replace(b":", b"/").decode()
        files.add(Path(path))

    return files


def check_bazel_build_for_files(
    bazel_extensions_to_check: Container[str],
    files: Iterable[Path],
    bazel_dirs: Iterable[Path],
    picotool_dir: Optional[Path],
) -> List[Path]:
    """Checks that source files are in the Bazel builds.

    Args:
        bazel_extensions_to_check: which file suffixes to look for in Bazel
        files: the files that should be checked
        bazel_dirs: directories in which to run bazel query

    Returns:
        a list of missing files; will be empty if there were no missing files
    """

    # Collect all paths in the Bazel builds files.
    bazel_build_source_files: Set[Path] = set()
    pictool_override = override_picotool_arg(picotool_dir) if picotool_dir else ""
    for directory in bazel_dirs:
        bazel_build_source_files.update(
            get_paths_from_command(
                directory, bazel_command(), "query", pictool_override, 'kind("source file", //...:*)',
            )
        )
    missing_from_bazel: List[Path] = []
    referenced_in_bazel_missing: List[Path] = []

    if not bazel_dirs:
        _LOG.error("No bazel directories to check.")
        raise RuntimeError

    for path in (p for p in files if p.suffix in bazel_extensions_to_check):
        if path not in bazel_build_source_files:
            missing_from_bazel.append(path)

    for path in (
        p for p in bazel_build_source_files if p.suffix in bazel_extensions_to_check
    ):
        if path not in files:
            referenced_in_bazel_missing.append(path)

    if missing_from_bazel:
        _LOG.warning(
            "Files not included in the Bazel build:\n\n%s\n",
            "\n".join("  " + str(x) for x in sorted(missing_from_bazel)),
        )

    if referenced_in_bazel_missing:
        _LOG.warning(
            "Files referenced in the Bazel build that are missing:\n\n%s\n",
            "\n".join("  " + str(x) for x in sorted(referenced_in_bazel_missing)),
        )

    return missing_from_bazel + referenced_in_bazel_missing


def git_ls_files_by_extension(file_suffixes: Iterable[str]) -> Iterable[Path]:
    """List git source files.

    Returns: A list of files matching the provided extensions.
    """
    git_command = ["git", "ls-files"]
    for pattern in file_suffixes:
        git_command.append("*" + pattern)

    bazel_file_list = subprocess.run(
        git_command,
        cwd=SDK_ROOT,
        text=True,
        check=True,
        capture_output=True,
    ).stdout

    bazel_files = [Path(f) for f in bazel_file_list.splitlines()]
    return bazel_files


def check_sources_in_bazel_build(picotool_dir) -> int:
    # List files using git ls-files
    all_source_files = git_ls_files_by_extension(
        CPP_HEADER_EXTENSIONS + CPP_SOURCE_EXTENSIONS
    )

    # Filter out any unwanted files.
    ignored_files = []
    for source in all_source_files:
        for pattern in IGNORED_FILE_PATTERNS:
            if source.match(pattern):
                ignored_files.append(source)
    _LOG.debug(
        "Ignoring files:\n\n%s\n", "\n".join("  " + str(f) for f in ignored_files)
    )

    source_files = list(set(all_source_files) - set(ignored_files))

    # Check for missing files.
    _LOG.info("Checking all source files are accounted for in Bazel.")
    missing_files = check_bazel_build_for_files(
        bazel_extensions_to_check=CPP_HEADER_EXTENSIONS + CPP_SOURCE_EXTENSIONS,
        files=source_files,
        bazel_dirs=[Path(SDK_ROOT)],
        picotool_dir=picotool_dir,
    )

    if missing_files:
        _LOG.error("Missing files found.")
        return 1

    _LOG.info("\x1b[32mSuccess!\x1b[0m All files accounted for in Bazel.")
    return 0


if __name__ == "__main__":
    setup_logging()
    args = parse_common_args()
    sys.exit(check_sources_in_bazel_build(args.picotool_dir))
