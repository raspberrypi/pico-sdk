#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Generate coverage report for pioasm and optional HTML output.

from __future__ import annotations

import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def _run(cmd, check=True, capture_output=False):
    return subprocess.run(
        cmd,
        check=check,
        text=True,
        capture_output=capture_output,
    )


def _bazel_info(key: str) -> str:
    result = _run(["bazel", "info", key], capture_output=True)
    return result.stdout.strip()


def _has_da(path: Path) -> bool:
    try:
        with path.open("r", encoding="utf-8") as handle:
            for line in handle:
                if line.startswith("DA:"):
                    return True
    except FileNotFoundError:
        return False
    return False


def _find_tool(name: str) -> str | None:
    try:
        result = _run(["xcrun", "-f", name], check=False, capture_output=True)
        candidate = result.stdout.strip()
        if result.returncode == 0 and candidate:
            return candidate
    except FileNotFoundError:
        pass
    return shutil.which(name)


def _collect_profraw(paths) -> list[Path]:
    profraw_files = []
    for base in paths:
        if not base or not base.exists():
            continue
        for root, _, files in os.walk(base):
            for filename in files:
                if filename.endswith(".profraw"):
                    profraw_files.append(Path(root) / filename)
    return profraw_files


def _rewrite_paths(coverage_path: Path, root_dir: Path) -> None:
    root_prefix = f"SF:{root_dir.as_posix()}/"
    fd, tmp_name = tempfile.mkstemp(prefix="pioasm-cov-rewrite.")
    os.close(fd)
    tmp_rewrite = Path(tmp_name)
    try:
        with coverage_path.open("r", encoding="utf-8") as src, tmp_rewrite.open(
            "w", encoding="utf-8"
        ) as dst:
            for line in src:
                if line.startswith("SF:"):
                    line = re.sub(r"^SF:.*/execroot/_main/", "SF:", line)
                    if line.startswith(root_prefix):
                        line = "SF:" + line[len(root_prefix):]
                dst.write(line)
        tmp_rewrite.replace(coverage_path)
    finally:
        if tmp_rewrite.exists():
            try:
                tmp_rewrite.unlink()
            except FileNotFoundError:
                pass


def main() -> int:
    if os.environ.get("BUILD_WORKSPACE_DIRECTORY"):
        root_dir = Path(os.environ["BUILD_WORKSPACE_DIRECTORY"]).resolve()
    else:
        root_dir = Path(__file__).resolve().parents[3]

    os.chdir(root_dir)
    _run(
        [
            "bazel",
            "coverage",
            "//tools/pioasm:pioasm_core_coverage_test",
            "--combined_report=lcov",
            "--cache_test_results=no",
        ]
    )

    coverage_src = root_dir / "bazel-out/_coverage/_coverage_report.dat"
    if not coverage_src.exists():
        output_base = Path(_bazel_info("output_base"))
        alt_src = output_base / "execroot/_main/bazel-out/_coverage/_coverage_report.dat"
        if alt_src.exists():
            coverage_src = alt_src
        else:
            print(
                f"Coverage report not found at {coverage_src} or {alt_src}",
                file=sys.stderr,
            )
            return 1

    work_dest = coverage_src
    temp_dir = None
    if not os.access(work_dest, os.W_OK):
        temp_dir = Path(tempfile.mkdtemp(prefix="pioasm-cov."))
        work_dest = temp_dir / "coverage.lcov"

    check_src = coverage_src if coverage_src.exists() else work_dest
    if not _has_da(check_src):
        llvm_cov = _find_tool("llvm-cov")
        llvm_profdata = _find_tool("llvm-profdata")
        if llvm_cov and llvm_profdata:
            bazel_bin = Path(_bazel_info("bazel-bin"))
            bazel_testlogs = Path(_bazel_info("bazel-testlogs"))
            test_bin = bazel_bin / "tools/pioasm/pioasm_core_coverage_test"
            output_base = Path(_bazel_info("output_base"))
            profraw_files = _collect_profraw(
                [
                    bazel_testlogs / "_coverage",
                    output_base / "sandbox/sandbox_stash/TestRunner",
                ]
            )

            if test_bin.exists() and profraw_files:
                tmp_dir = Path(tempfile.mkdtemp(prefix="pioasm-cov-merge."))
                profdata_path = tmp_dir / "coverage.profdata"
                _run(
                    [
                        llvm_profdata,
                        "merge",
                        "-sparse",
                        *[str(p) for p in profraw_files],
                        "-o",
                        str(profdata_path),
                    ]
                )
                with work_dest.open("w", encoding="utf-8") as handle:
                    subprocess.run(
                        [
                            llvm_cov,
                            "export",
                            "-format=lcov",
                            f"-instr-profile={profdata_path}",
                            str(test_bin),
                        ],
                        check=True,
                        text=True,
                        stdout=handle,
                    )
                shutil.rmtree(tmp_dir, ignore_errors=True)

    coverage_dir = root_dir / "tools/pioasm/test/coverage"
    coverage_dir.mkdir(parents=True, exist_ok=True)
    coverage_dest = coverage_dir / "coverage.lcov"
    if coverage_dest.exists():
        try:
            coverage_dest.chmod(coverage_dest.stat().st_mode | 0o200)
        except OSError:
            pass

    source_path = work_dest if work_dest.exists() else coverage_src
    shutil.copyfile(source_path, coverage_dest)
    print(f"Coverage report written to {coverage_dest}")

    _rewrite_paths(coverage_dest, root_dir)

    genhtml_bin = shutil.which("genhtml")
    if genhtml_bin:
        html_dir = coverage_dir / "html"
        result = subprocess.run(
            [
                genhtml_bin,
                str(coverage_dest),
                "-o",
                str(html_dir),
                "--ignore-errors",
                "inconsistent,unsupported",
            ],
            text=True,
        )
        if result.returncode == 0:
            print(f"HTML coverage written to {html_dir}")
        else:
            print("genhtml failed; HTML coverage not generated", file=sys.stderr)

    if temp_dir:
        shutil.rmtree(temp_dir, ignore_errors=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
