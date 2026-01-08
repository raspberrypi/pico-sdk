#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Test harness for pioasm validation and coverage tests.
#
# Usage:
# test/run_tests.py

import os, sys, subprocess, tempfile, shutil
from pathlib import Path

TEST_DIR = Path(".") / "test"

def discover_tests() -> list[Path]:
	for (root, dirs, files) in TEST_DIR.walk():
		return [root / file for file in files if file.startswith("test_") and file.endswith(".pio")]

	return []

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

	if overwrite:
		# Rewrite the test file with new output
		with open(test, "w") as test_file:
			for line in lines:
				test_file.write(line)
			for output_line in commands_output:
				test_file.write(output_line + "\n")
	else:
		# Compare output
		expected_output = []
		for output_line in output_lines:
			expected_output.append(strip_trailing_newline(output_line))

		if expected_output != commands_output:
			print("Test failed!")
			#print a diff instead
			#an actual diff that is not broken if there is a new / removed line. maybe use a lib
			import difflib
			diff = difflib.unified_diff(expected_output, commands_output, fromfile='expected', tofile='actual', lineterm='')
			print('\n'.join(diff))

			# print("Expected output:")
			# print("\n".join(expected_output))
			# print("Actual output:")
			# print("\n".join(commands_output))
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

	# path=foobar:$PATH
	env = os.environ.copy()
	env["PATH"] = f"/Users/gonzalo/.pico-sdk/tools/2.2.0/pioasm:{env['PATH']}"
	result = subprocess.run(command, cwd=tempdir, env=env, capture_output=True, shell=True)
	
	return (result.returncode, result.stdout.decode(), result.stderr.decode())

def run_tests(overwrite: bool = False):
	tests = discover_tests()
	print(tests)
	all_passed = True

	for test in tests:
		print(f"Running test {test}: ", end="")
		if not run_test(test, overwrite):
			all_passed = False
		else:
			print("OK")

	return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(run_tests(sys.argv[1:] == ["--overwrite"]))
