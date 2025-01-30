#!/usr/bin/env python3

import os
from collections import OrderedDict
import subprocess
import re

toolchains = [os.path.join("/opt/arm", x) for x in os.listdir("/opt/arm")]
toolchains += [os.path.join("/opt/riscv", x) for x in os.listdir("/opt/riscv")]

compilers = []
class Compiler:
    def __init__(self, version, path, type):
        self.version = version
        self.path = path
        self.type = type

    @property
    def gcc(self):
        return self.type == "GCC"

    @property
    def llvm(self):
        return self.type == "LLVM"

    @property
    def riscv(self):
        return "RISCV" in self.type

    def __repr__(self):
        return self.version

seen_versions = []
for toolchain in toolchains:
    gcc_path = os.path.join(toolchain, "bin/arm-none-eabi-gcc")
    llvm_path = os.path.join(toolchain, "bin/clang")
    riscv_gcc_path = os.path.join(toolchain, "bin/riscv32-unknown-elf-gcc")

    type = None
    path = None
    if os.path.exists(gcc_path):
        path = gcc_path
        type = "GCC"
    elif os.path.exists(llvm_path):
        path = llvm_path
        type = "LLVM"
    elif os.path.exists(riscv_gcc_path):
        path = riscv_gcc_path
        type = "RISCV GCC"
    else:
        raise Exception("Unknown compiler type")

    version = subprocess.run([path, "--version"], capture_output=True)
    stdout = version.stdout.decode('utf-8')
    stderr = version.stderr.decode('utf-8')
    assert(len(stderr) == 0)
    # Version should be on first line
    version_line = stdout.split("\n")[0]
    m = re.search("(\d+\.\d+\.\d+)", version_line)
    assert(m is not None)
    version = m.group(1)

    if version in seen_versions:
        raise Exception("Already have version {} in versions current path {}, this path {}".format(version, gcc_versions[version], path))

    compilers.append(Compiler(version, toolchain, type))
    seen_versions.append(version)

compilers_sorted = sorted(compilers, key=lambda x: int(x.version.replace(".", "")))

# Create output
output = '''
name: Multi GCC
on:
  workflow_dispatch:
  push:
    branches:
      - 'master'
      - 'test_workflow'

jobs:
  build:
    if: github.repository_owner == 'raspberrypi'
    runs-on: [self-hosted, Linux, X64]

    steps:
    - name: Clean workspace
      run: |
        echo "Cleaning up previous run"
        rm -rf "${{ github.workspace }}"
        mkdir -p "${{ github.workspace }}"

    - name: Checkout repo
      uses: actions/checkout@v4

    - name: Checkout submodules
      run: git submodule update --init

    - name: Host Release
      run: cd ${{github.workspace}}; mkdir -p build; rm -rf build/*; cd build; cmake ../ -DPICO_SDK_TESTS_ENABLED=1 -DCMAKE_BUILD_TYPE=Release -DPICO_NO_PICOTOOL=1 -DPICO_PLATFORM=host; make --output-sync=target --no-builtin-rules --no-builtin-variables -j$(nproc)

    - name: Host Debug
      run: cd ${{github.workspace}}; mkdir -p build; rm -rf build/*; cd build; cmake ../ -DPICO_SDK_TESTS_ENABLED=1 -DCMAKE_BUILD_TYPE=Debug -DPICO_NO_PICOTOOL=1 -DPICO_PLATFORM=host; make --output-sync=target --no-builtin-rules --no-builtin-variables -j$(nproc)
'''

platforms = []
class Platform:
    def __init__(self, name, platform, board, minimum_gcc_version=None):
        self.name = name
        self.board = board
        self.platform = platform
        self.riscv = "riscv" in platform
        self.minimum_gcc_version = minimum_gcc_version

    def cmake_string(self, compiler):
        opts = []
        # Temporary while private repo
        opts.append("-DPICO_NO_PICOTOOL=1")
        if self.board: opts.append(f"-DPICO_BOARD={self.board}")
        opts.append(f"-DPICO_PLATFORM={self.platform}")
        if compiler.llvm: opts.append("-DPICO_COMPILER=pico_arm_clang")
        opts.append(f"-DPICO_TOOLCHAIN_PATH={compiler.path}")
        return " ".join(opts)

    def compiler_valid(self, compiler):
        if compiler.riscv != self.riscv:
            return False

        if self.minimum_gcc_version and compiler.gcc:
            if int(compiler.version.split(".")[0]) < self.minimum_gcc_version:
                return False

        return True


platforms.append(Platform("Pico W", "rp2040", "pico_w"))
platforms.append(Platform("RP2350", "rp2350", None, 9))
platforms.append(Platform("RP2350 RISCV", "rp2350-riscv", None))

for compiler in compilers_sorted:
    for build_type in ["Debug", "Release"]:
        for p in platforms:
            if not p.compiler_valid(compiler): continue
            output += "\n"
            output += "    - name: {} {} {} {}\n".format(compiler.type, compiler.version, build_type, p.name)
            output += "      if: always()\n"
            output += "      shell: bash\n"
            output += "      run: cd ${{{{github.workspace}}}}; mkdir -p build; rm -rf build/*; cd build; cmake ../ -DPICO_SDK_TESTS_ENABLED=1 -DCMAKE_BUILD_TYPE={} {}; make --output-sync=target --no-builtin-rules --no-builtin-variables -j$(nproc)\n".format(build_type, p.cmake_string(compiler))
print(output)
