#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
  root_dir="$BUILD_WORKSPACE_DIRECTORY"
else
  root_dir=$(cd "$(dirname "$0")/../../.." && pwd)
fi

cd "$root_dir"
bazel coverage //tools/pioasm:pioasm_core_coverage_test --combined_report=lcov --cache_test_results=no

coverage_src="$root_dir/bazel-out/_coverage/_coverage_report.dat"
coverage_dir="$root_dir/tools/pioasm/test/coverage"
coverage_dest="$coverage_dir/coverage.lcov"
work_dest="$coverage_src"
temp_dir=""

if [[ ! -f "$coverage_src" ]]; then
  output_base=$(bazel info output_base)
  alt_src="$output_base/execroot/_main/bazel-out/_coverage/_coverage_report.dat"
  if [[ -f "$alt_src" ]]; then
    coverage_src="$alt_src"
    work_dest="$coverage_src"
  else
    echo "Coverage report not found at $coverage_src or $alt_src" >&2
    exit 1
  fi
fi

if [[ ! -w "$work_dest" ]]; then
  temp_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioasm-cov.XXXXXX")
  work_dest="$temp_dir/coverage.lcov"
fi

check_src="$coverage_src"
if [[ ! -f "$check_src" ]]; then
  check_src="$work_dest"
fi

if ! rg -q "^DA:" "$check_src"; then
  llvm_cov=""
  llvm_profdata=""
  if command -v xcrun >/dev/null 2>&1; then
    llvm_cov=$(xcrun -f llvm-cov 2>/dev/null || true)
    llvm_profdata=$(xcrun -f llvm-profdata 2>/dev/null || true)
  fi
  if [[ -z "$llvm_cov" ]]; then
    llvm_cov=$(command -v llvm-cov 2>/dev/null || true)
  fi
  if [[ -z "$llvm_profdata" ]]; then
    llvm_profdata=$(command -v llvm-profdata 2>/dev/null || true)
  fi

  if [[ -n "$llvm_cov" && -n "$llvm_profdata" ]]; then
    bazel_bin=$(bazel info bazel-bin)
    bazel_testlogs=$(bazel info bazel-testlogs)
    test_bin="$bazel_bin/tools/pioasm/pioasm_core_coverage_test"
    profraw_files=()
    if [[ -d "$bazel_testlogs/_coverage" ]]; then
      while IFS= read -r -d '' file; do
        profraw_files+=("$file")
      done < <(find "$bazel_testlogs/_coverage" -name '*.profraw' -print0)
    fi
    if [[ ${#profraw_files[@]} -eq 0 ]]; then
      output_base=$(bazel info output_base)
      if [[ -d "$output_base/sandbox/sandbox_stash/TestRunner" ]]; then
        while IFS= read -r -d '' file; do
          profraw_files+=("$file")
        done < <(find "$output_base/sandbox/sandbox_stash/TestRunner" -name '*.profraw' -print0)
      fi
    fi

    if [[ -x "$test_bin" && ${#profraw_files[@]} -gt 0 ]]; then
      tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/pioasm-cov.XXXXXX")
      profdata_path="$tmp_dir/coverage.profdata"
      "$llvm_profdata" merge -sparse "${profraw_files[@]}" -o "$profdata_path"
      "$llvm_cov" export -format=lcov -instr-profile="$profdata_path" "$test_bin" > "$work_dest"
      rm -rf "$tmp_dir"
    fi
  fi
fi

mkdir -p "$coverage_dir"
if [[ -f "$coverage_dest" ]]; then
  chmod u+w "$coverage_dest" || true
fi
if cp -f "$work_dest" "$coverage_dest"; then
  echo "Coverage report written to $coverage_dest"
else
  echo "Coverage report written to $work_dest"
fi

tmp_rewrite=$(mktemp "${TMPDIR:-/tmp}/pioasm-cov-rewrite.XXXXXX")
perl -pe "s{^SF:.*/execroot/_main/}{SF:}s; s{^SF:\\Q$root_dir\\E/}{SF:}s" "$coverage_dest" > "$tmp_rewrite"
mv "$tmp_rewrite" "$coverage_dest"

genhtml_bin=$(command -v genhtml 2>/dev/null || true)
if [[ -n "$genhtml_bin" ]]; then
  html_dir="$coverage_dir/html"
  "$genhtml_bin" "$coverage_dest" -o "$html_dir" --ignore-errors inconsistent,unsupported || true
  echo "HTML coverage written to $html_dir"
fi

if [[ -n "$temp_dir" ]]; then
  rm -rf "$temp_dir"
fi
