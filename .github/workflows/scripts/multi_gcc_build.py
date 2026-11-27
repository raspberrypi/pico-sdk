#!/usr/bin/env python3
"""
Build the SDK tests with every available toolchain.

Toolchains are discovered under /opt/arm and /opt/riscv. Each build is run in
sequence, and failures don't stop the remaining builds. Output uses GitHub
Actions log groups and error annotations, and a results table is written to
the job summary.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

DEFAULT_TOOLCHAIN_ROOTS = ["/opt/arm", "/opt/riscv"]
BUILD_TYPES = ["Debug", "Release"]


class Compiler:
    def __init__(self, type, version, path):
        self.type = type
        self.version = version
        self.path = path

    @property
    def gcc(self):
        return self.type == "GCC"

    @property
    def llvm(self):
        return self.type == "LLVM"

    @property
    def riscv(self):
        return "RISCV" in self.type

    @property
    def version_key(self):
        return tuple(int(x) for x in self.version.split("."))

    @property
    def name(self):
        return f"{self.type} {self.version}"


class Platform:
    def __init__(self, name, platform, board=None, minimum_gcc_version=None):
        self.name = name
        self.platform = platform
        self.board = board
        self.riscv = "riscv" in platform
        self.minimum_gcc_version = minimum_gcc_version

    def compiler_valid(self, compiler):
        if compiler.riscv != self.riscv:
            return False
        if self.minimum_gcc_version and compiler.gcc:
            if compiler.version_key[0] < self.minimum_gcc_version:
                return False
        return True

    def cmake_args(self, compiler):
        args = ["-DPICO_NO_PICOTOOL=1"]
        if self.board:
            args.append(f"-DPICO_BOARD={self.board}")
        args.append(f"-DPICO_PLATFORM={self.platform}")
        if compiler.llvm:
            args.append("-DPICO_COMPILER=pico_arm_clang")
        args.append(f"-DPICO_TOOLCHAIN_PATH={compiler.path}")
        return args


PLATFORMS = [
    # Pico W is broken on GCC 6 and 7
    Platform("Pico W", "rp2040", "pico_w", minimum_gcc_version=8),
    Platform("RP2350", "rp2350", minimum_gcc_version=9),
    Platform("RP2350 RISCV", "rp2350-riscv"),
]


class Build:
    def __init__(self, name, build_type, cmake_args):
        self.name = name
        self.build_type = build_type
        self.cmake_args = cmake_args
        self.result = None
        self.duration = None

    @property
    def cmake_command(self):
        return ["cmake", "../", "-DPICO_SDK_TESTS_ENABLED=1",
                f"-DCMAKE_BUILD_TYPE={self.build_type}"] + self.cmake_args


def escape_data(s):
    return s.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(s):
    return escape_data(s).replace(":", "%3A").replace(",", "%2C")


def log(*args):
    print(*args, flush=True)


def error(title, message):
    log(f"::error title={escape_property(title)}::{escape_data(message)}")


def detect_compiler(toolchain):
    candidates = [
        ("GCC", "bin/arm-none-eabi-gcc"),
        ("LLVM", "bin/clang"),
        ("RISCV GCC", "bin/riscv32-pico-elf-gcc"),
        ("RISCV GCC", "bin/riscv32-unknown-elf-gcc"),
    ]
    for type, binary in candidates:
        path = os.path.join(toolchain, binary)
        if os.path.exists(path):
            break
    else:
        raise Exception(f"Unknown compiler type in {toolchain}")

    result = subprocess.run([path, "--version"], capture_output=True)
    stdout = result.stdout.decode("utf-8")
    stderr = result.stderr.decode("utf-8")
    if result.returncode != 0:
        raise Exception(f"{path} --version failed with exit code {result.returncode}:\n{stderr}")
    if stderr:
        raise Exception(f"Unexpected stderr from {path} --version:\n{stderr}")
    # Version should be on first line
    m = re.search(r"(\d+\.\d+\.\d+)", stdout.split("\n")[0])
    if m is None:
        raise Exception(f"Couldn't find version in output of {path} --version:\n{stdout}")
    return Compiler(type, m.group(1), toolchain)


def find_compilers(roots):
    compilers = []
    seen = {}
    for root in roots:
        if not os.path.isdir(root):
            continue
        for entry in sorted(os.listdir(root)):
            compiler = detect_compiler(os.path.join(root, entry))
            key = (compiler.type, compiler.version)
            if key in seen:
                raise Exception(f"Already have {compiler.name} at {seen[key]}, this path {compiler.path}")
            seen[key] = compiler.path
            compilers.append(compiler)
    return sorted(compilers, key=lambda c: (c.version_key, c.type))


def select_quick_compilers(compilers):
    """Pick the oldest compiler each platform supports, and the newest, of each type."""
    selected = set()
    for type in sorted({c.type for c in compilers}):
        of_type = [c for c in compilers if c.type == type]
        selected.add(of_type[-1].path)
        for p in PLATFORMS:
            valid = [c for c in of_type if p.compiler_valid(c)]
            if valid:
                selected.add(valid[0].path)
    return [c for c in compilers if c.path in selected]


def plan_builds(compilers, mode):
    builds = []
    if mode == "quick":
        for build_type in ["Release", "Debug"]:
            builds.append(Build(f"Host {build_type}", build_type,
                                ["-DPICO_NO_PICOTOOL=1", "-DPICO_PLATFORM=host"]))
        compilers = select_quick_compilers(compilers)
        build_types = ["Debug"]
    else:
        build_types = BUILD_TYPES

    for compiler in compilers:
        for build_type in build_types:
            for p in PLATFORMS:
                if p.compiler_valid(compiler):
                    builds.append(Build(f"{compiler.name} {build_type} {p.name}",
                                        build_type, p.cmake_args(compiler)))
    return builds


def compiler_platforms(compiler):
    names = [p.name for p in PLATFORMS if p.compiler_valid(compiler)]
    return ", ".join(names) if names else "none (skipped)"


def print_toolchains(compilers):
    log(f"Found {len(compilers)} toolchains:")
    rows = [(c.type, c.version, c.path, compiler_platforms(c)) for c in compilers]
    widths = [max([len(r[i]) for r in rows] + [0]) for i in range(3)]
    for row in rows:
        log("  " + "  ".join(col.ljust(w) for col, w in zip(row, widths)) + "  " + row[3])
    log()


def run_build(build, source_dir, jobs):
    build_dir = os.path.join(source_dir, "build")
    shutil.rmtree(build_dir, ignore_errors=True)
    os.makedirs(build_dir)

    make_command = ["make", "--output-sync=target", "--no-builtin-rules",
                    "--no-builtin-variables", f"-j{jobs}"]
    for stage, command in [("cmake", build.cmake_command), ("make", make_command)]:
        log("$ " + " ".join(command))
        rc = subprocess.run(command, cwd=build_dir).returncode
        if rc != 0:
            return f"{stage} failed with exit code {rc}"
    return None


def format_duration(seconds):
    minutes, seconds = divmod(int(round(seconds)), 60)
    return f"{minutes}m {seconds:02d}s" if minutes else f"{seconds}s"


def write_summary(path, mode, compilers, builds):
    finished = [b for b in builds if b.result is not None]
    failed = [b for b in finished if b.result != "passed"]
    lines = [f"## Multi GCC ({mode})", ""]
    lines.append(f"{len(finished) - len(failed)} passed, {len(failed)} failed"
                 + (f", {len(builds) - len(finished)} not run" if len(finished) < len(builds) else ""))
    lines += ["", "| Build | Result | Duration |", "| --- | --- | --- |"]
    for b in finished:
        result = "✅ Passed" if b.result == "passed" else f"❌ {b.result}"
        lines.append(f"| {b.name} | {result} | {format_duration(b.duration)} |")
    lines += ["", "<details><summary>Toolchains found</summary>", "",
              "| Type | Version | Path | Platforms |", "| --- | --- | --- | --- |"]
    for c in compilers:
        lines.append(f"| {c.type} | {c.version} | `{c.path}` | {compiler_platforms(c)} |")
    lines += ["", "</details>", ""]
    with open(path, "a", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["quick", "exhaustive"])
    parser.add_argument("--dry-run", action="store_true",
                        help="list the toolchains and builds without running anything")
    parser.add_argument("--toolchain-root", action="append", dest="toolchain_roots",
                        help="directory containing toolchains (default: %s)" % " and ".join(DEFAULT_TOOLCHAIN_ROOTS))
    parser.add_argument("--source-dir", default=os.getcwd(), help="SDK source directory (default: current directory)")
    args = parser.parse_args()

    compilers = find_compilers(args.toolchain_roots or DEFAULT_TOOLCHAIN_ROOTS)
    print_toolchains(compilers)

    ok = True
    for p in PLATFORMS:
        if not any(p.compiler_valid(c) for c in compilers):
            error("Missing toolchain", f"No toolchains found for {p.name}")
            ok = False

    builds = plan_builds(compilers, args.mode)
    log(f"Planned {len(builds)} {args.mode} builds:")
    for b in builds:
        log(f"  {b.name}")
    log()

    if args.dry_run:
        for b in builds:
            log(f"{b.name}: {' '.join(b.cmake_command)}")
        return 0 if ok else 1

    jobs = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else os.cpu_count()
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    try:
        for b in builds:
            log(f"::group::{b.name}")
            start = time.monotonic()
            failure = run_build(b, args.source_dir, jobs)
            b.duration = time.monotonic() - start
            b.result = failure or "passed"
            log("::endgroup::")
            if failure:
                error(b.name, failure)
    finally:
        if summary_path:
            write_summary(summary_path, args.mode, compilers, builds)

    failed = [b for b in builds if b.result != "passed"]
    log(f"{len(builds) - len(failed)} of {len(builds)} builds passed")
    if failed:
        log("Failed builds:")
        for b in failed:
            log(f"  {b.name}: {b.result}")
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
