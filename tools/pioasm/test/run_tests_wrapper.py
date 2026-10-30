#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Wrapper to locate runfiles and invoke run_tests.py under Bazel.

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys


RUN_TESTS_RUNFILE = "_main/tools/pioasm/test/run_tests.py"
PIOASM_RUNFILE = "_main/tools/pioasm/pioasm"


def _resolve_runfile(path: str) -> Path | None:
    runfiles_dir = os.environ.get("RUNFILES_DIR")
    if runfiles_dir:
        candidate = Path(runfiles_dir) / path
        if candidate.exists():
            return candidate

    manifest = os.environ.get("RUNFILES_MANIFEST_FILE")
    if manifest:
        prefix = f"{path} "
        with open(manifest, "r", encoding="utf-8") as handle:
            for line in handle:
                if line.startswith(prefix):
                    return Path(line[len(prefix):].strip())

    return None


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    test_script = _resolve_runfile(RUN_TESTS_RUNFILE) or (script_dir / "run_tests.py")
    pioasm_path = _resolve_runfile(PIOASM_RUNFILE)

    if not test_script.exists():
        print(f"run_tests.py not found at {test_script}", file=sys.stderr)
        return 1

    env = os.environ.copy()
    if pioasm_path:
        env["PIOASM_BIN"] = str(pioasm_path)

    python_bin = env.get("PYTHON_BIN", sys.executable)
    cmd = [python_bin, str(test_script), *sys.argv[1:]]
    return subprocess.run(cmd, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
