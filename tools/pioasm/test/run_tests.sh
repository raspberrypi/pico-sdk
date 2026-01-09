#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
RUNFILES_PATH="_main/tools/pioasm/test/run_tests.py"
PIOASM_RUNFILES_PATH="_main/tools/pioasm/pioasm"

if [[ -n "${RUNFILES_DIR:-}" ]]; then
  TEST_SCRIPT="$RUNFILES_DIR/$RUNFILES_PATH"
  PIOASM_PATH="$RUNFILES_DIR/$PIOASM_RUNFILES_PATH"
elif [[ -n "${RUNFILES_MANIFEST_FILE:-}" ]]; then
  TEST_SCRIPT=$(grep -m1 "^${RUNFILES_PATH} " "$RUNFILES_MANIFEST_FILE" | cut -d' ' -f2-)
  PIOASM_PATH=$(grep -m1 "^${PIOASM_RUNFILES_PATH} " "$RUNFILES_MANIFEST_FILE" | cut -d' ' -f2-)
else
  TEST_SCRIPT="$SCRIPT_DIR/run_tests.py"
  PIOASM_PATH=""
fi

if [[ -n "${PIOASM_PATH:-}" && -x "${PIOASM_PATH}" ]]; then
  export PIOASM_BIN="$PIOASM_PATH"
elif [[ -z "${PIOASM_BIN:-}" && -n "${PIOASM_PATH:-}" ]]; then
  export PIOASM_BIN="$PIOASM_PATH"
fi

"${PYTHON_BIN:-python3}" "$TEST_SCRIPT"
