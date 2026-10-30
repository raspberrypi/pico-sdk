#!/usr/bin/env python3
#
# Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Test harness for pioasm validation and coverage tests.
#
# Usage:
# test/run_tests.py

import os, sys, subprocess, tempfile, shutil, re, unittest
from typing import Optional
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
OVERWRITE = False
PIOASM_BIN = None

def discover_tests() -> list[Path]:
	tests = []
	for root, _dirs, files in os.walk(TEST_DIR):
		for file in files:
			if file.startswith("test_") and file.endswith(".pio"):
				tests.append(Path(root) / file)
	return tests

def run_test(test: Path, overwrite: bool = False) -> bool:
	with open(test, "r") as test_file:
		line = test_file.readline()
		lines = [line]
		output_lines = []
		commands = []
		
		while line.startswith("// run: "):
			commands.append(line.strip()[8:])
			line = test_file.readline()
			lines.append(line)

		marker_found = False
		while not line == "" and not line.strip() == "// -- Output":
			if "// -- Output" in line:
				prefix, _ = line.split("// -- Output", 1)
				if prefix == "":
					lines[-1] = "// -- Output\n"
				else:
					lines[-1] = prefix if prefix.endswith("\n") else prefix + "\n"
					lines.append("// -- Output\n")
				marker_found = True
				break

			line = test_file.readline()
			lines.append(line)

		if line == "" and not marker_found:
			if overwrite:
				if lines and lines[-1] == "":
					lines.pop()
				if lines and not lines[-1].endswith("\n"):
					lines[-1] += "\n"
				lines.append("// -- Output\n")
			else:
				return False
		else:
			if not marker_found and (not lines or lines[-1].strip() != "// -- Output"):
				lines.append(line)
			line = test_file.readline()

			while line != "":
				output_lines.append(line)
				line = test_file.readline()

		commands_output = []

		for command in commands:
			(code, out, err) = execute_command(test, command)

			commands_output.append(f"// Command: {command}")
			commands_output.append(f"// Exit code: {code}")
			commands_output.extend(format_stream_output("Stdout:", out))
			commands_output.extend(format_stream_output("Stderr:", err))

	expected_output = []
	for output_line in output_lines:
		expected_output.append(strip_trailing_newline(output_line))

	if overwrite:
		# Rewrite the test file with new output, preserving wildcard lines.
		merged_output = list(commands_output)
		for index, expected_line in enumerate(expected_output):
			if index >= len(merged_output):
				break
			if expected_line.startswith("//?"):
				merged_output[index] = expected_line
		with open(test, "w") as test_file:
			for line in lines:
				test_file.write(line)
			for output_line in merged_output:
				test_file.write(output_line + "\n")
	else:
		# Compare output
		if not outputs_match(expected_output, commands_output):
			print("Test failed!")
			#print a diff instead
			#an actual diff that is not broken if there is a new / removed line. maybe use a lib
			import difflib
			diff = difflib.unified_diff(expected_output, commands_output, fromfile='expected', tofile='actual', lineterm='')
			print('\n'.join(diff))

			return False

	return True

def outputs_match(expected: list[str], actual: list[str]) -> bool:
	if len(expected) != len(actual):
		return False
	for expected_line, actual_line in zip(expected, actual):
		if expected_line.startswith("//?"):
			continue
		if expected_line != actual_line:
			return False
	return True

def strip_trailing_newline(line: str) -> str:
	if line.endswith("\n"):
		return line[:-1]
	return line

def format_stream_output(label: str, output: str) -> list[str]:
	lines = [f"// {label}"]
	if output == "":
		return lines

	for line in output.splitlines():
		lines.append(f"// {line}")

	if output.endswith("\n"):
		lines.append("// ")

	return lines

def execute_command(test_file, command) -> tuple[int, str, str]:
	tempdir = tempfile.mkdtemp()

	shutil.copy(test_file, Path(tempdir) / "input.pio")

	env = os.environ.copy()
	coverage_dir = env.get("COVERAGE_DIR") or env.get("TEST_UNDECLARED_OUTPUTS_DIR")
	if coverage_dir:
		env.setdefault("GCOV_PREFIX", coverage_dir)
		env.setdefault("GCOV_PREFIX_STRIP", "0")
		env.setdefault("LLVM_PROFILE_FILE", str(Path(coverage_dir) / "pioasm-%p.profraw"))
	if PIOASM_BIN:
		pioasm_dir = str(Path(PIOASM_BIN).parent)
		env["PATH"] = f"{pioasm_dir}{os.pathsep}{env.get('PATH', '')}"
	result = subprocess.run(command, cwd=tempdir, env=env, capture_output=True, shell=True)
	
	return (result.returncode, result.stdout.decode(), result.stderr.decode())

class PioasmTests(unittest.TestCase):
	pass

def _make_test(test_path: Path):
	def _test(self):
		ok = run_test(test_path, OVERWRITE)
		self.assertTrue(ok, f"Test failed: {test_path}")
	return _test

def _register_tests():
	tests = discover_tests()
	for index, test_path in enumerate(tests):
		rel_path = test_path.relative_to(TEST_DIR)
		safe_name = re.sub(r"[^0-9A-Za-z_]+", "_", str(rel_path))
		test_name = f"test_{safe_name}_{index}"
		setattr(PioasmTests, test_name, _make_test(test_path))

_register_tests()

def resolve_pioasm_bin() -> Optional[str]:
	env_bin = os.environ.get("PIOASM_BIN")
	if env_bin:
		env_path = Path(env_bin)
		if env_path.is_absolute():
			return str(env_path)
		runfiles_dir = os.environ.get("RUNFILES_DIR")
		if runfiles_dir:
			candidate = Path(runfiles_dir) / env_path
			if candidate.exists():
				return str(candidate)
			candidate = Path(runfiles_dir) / "_main" / env_path
			if candidate.exists():
				return str(candidate)
		manifest = os.environ.get("RUNFILES_MANIFEST_FILE")
		if manifest:
			with open(manifest, "r") as manifest_file:
				for line in manifest_file:
					if line.startswith(f"{env_path} "):
						return line.split(" ", 1)[1].strip()
					if line.startswith(f"_main/{env_path} "):
						return line.split(" ", 1)[1].strip()
		for parent in Path(__file__).resolve().parents:
			if parent.name == "_main":
				candidate = parent.parent / env_path
				if candidate.exists():
					return str(candidate)
				candidate = parent / env_path
				if candidate.exists():
					return str(candidate)
				break
		for parent in Path(__file__).resolve().parents:
			if parent.name == "execroot":
				candidate = parent / env_path
				if candidate.exists():
					return str(candidate)
				break
		repo_root = Path(__file__).resolve().parents[3]
		candidate = repo_root / env_path
		if candidate.exists():
			return str(candidate)
		return env_bin

	repo_root = Path(__file__).resolve().parents[3]
	candidate = repo_root / "bazel-bin" / "tools" / "pioasm" / "pioasm"
	if candidate.exists():
		return str(candidate)

	return None

PIOASM_BIN = resolve_pioasm_bin()

if __name__ == "__main__":
	if "--overwrite" in sys.argv:
		OVERWRITE = True
		sys.argv.remove("--overwrite")
	unittest.main(verbosity=2)
