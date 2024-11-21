#!/usr/bin/env python3
#
# Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A script that verifies various Bazel build configurations succeed.

import logging
import os
import sys

from bazel_common import (
    SDK_ROOT,
    override_picotool_arg,
    parse_common_args,
    print_framed_string,
    print_to_stderr,
    run_bazel,
    setup_logging,
)


BUILD_CONFIGURATIONS = (
    {
        "name": "host",
        "args": (),
        "extra_targets": (
            "@picotool//:picotool",
            "//tools/pioasm:pioasm",
        ),
        "exclusions": frozenset(
            (
                "//test/cmsis_test:cmsis_test",
                "//test/hardware_irq_test:hardware_irq_test",
                "//test/hardware_pwm_test:hardware_pwm_test",
                "//test/hardware_sync_spin_lock_test:hardware_sync_spin_lock_test",
                "//test/kitchen_sink:kitchen_sink",
                "//test/kitchen_sink:kitchen_sink_cpp",
                "//test/kitchen_sink:kitchen_sink_lwip_poll",
                "//test/kitchen_sink:kitchen_sink_lwip_background",
                "//test/pico_divider_test:pico_divider_test",
                "//test/pico_divider_test:pico_divider_nesting_test",
                "//test/pico_float_test:pico_double_test",
                "//test/pico_float_test:pico_float_test",
                "//test/pico_float_test:pico_float_test_hazard3",
                "//test/pico_sha256_test:pico_sha256_test",
                "//test/pico_stdio_test:pico_stdio_test",
                "//test/pico_time_test:pico_time_test",

                # Pretty much only Picotool and pioasm build on Windows.
                "//..." if os.name == "nt" else "",
                "//test/pico_sem_test:pico_sem_test" if os.name == "nt" else "",
                "//test/pico_stdlib_test:pico_stdlib_test" if os.name == "nt" else "",

            )
        ),
        "run_targets": (""),
    },
    {
        "name": "rp2040",
        "args": ("--platforms=//bazel/platform:rp2040",),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                "//test/kitchen_sink:kitchen_sink_lwip_poll",
                "//test/kitchen_sink:kitchen_sink_lwip_background",
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # No RISC-V on RP2040.
                "//test/pico_float_test:pico_float_test_hazard3",
                # hardware_sha256 doesn't appear to work on RP2040.
                "//test/pico_sha256_test:pico_sha256_test",
            )
        ),
    },
    {
        "name": "rp2350",
        "args": ("--platforms=//bazel/platform:rp2350",),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                "//test/kitchen_sink:kitchen_sink_lwip_poll",
                "//test/kitchen_sink:kitchen_sink_lwip_background",
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # TODO: RISC-V support.
                "//test/pico_float_test:pico_float_test_hazard3",
            )
        ),
    },
    {
        "name": "rp2040 clang",
        "args": (
            "--platforms=//bazel/platform:rp2040",
            "--@pico-sdk//bazel/config:PICO_TOOLCHAIN=clang",
        ),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                "//test/kitchen_sink:kitchen_sink_lwip_poll",
                "//test/kitchen_sink:kitchen_sink_lwip_background",
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # No RISC-V on RP2040.
                "//test/pico_float_test:pico_float_test_hazard3",
                # hardware_sha256 doesn't appear to work on RP2040.
                "//test/pico_sha256_test:pico_sha256_test",
            )
        ),
    },
    {
        "name": "rp2350 clang",
        "args": (
            "--platforms=//bazel/platform:rp2350",
            "--@pico-sdk//bazel/config:PICO_TOOLCHAIN=clang",
        ),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                "//test/kitchen_sink:kitchen_sink_lwip_poll",
                "//test/kitchen_sink:kitchen_sink_lwip_background",
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # TODO: RISC-V support.
                "//test/pico_float_test:pico_float_test_hazard3",
            )
        ),
    },
    {
        "name": "Pico W",
        "args": (
            "--platforms=//bazel/platform:rp2040",
            "--@pico-sdk//bazel/config:PICO_BOARD=pico_w",
        ),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # No RISC-V on RP2040.
                "//test/pico_float_test:pico_float_test_hazard3",
                # hardware_sha256 doesn't appear to work on RP2040.
                "//test/pico_sha256_test:pico_sha256_test",
            )
        ),
    },
    {
        "name": "Pico 2 W",
        "args": (
            "--platforms=//bazel/platform:rp2350",
            "--@pico-sdk//bazel/config:PICO_BOARD=pico2_w",
        ),
        "extra_targets": (),
        "exclusions": frozenset(
            (
                # Host only.
                "//test/pico_float_test:hazard3_test_gen",
                # No RISC-V on RP2040.
                "//test/pico_float_test:pico_float_test_hazard3",
            )
        ),
    },
)


def _find_tests(picotool_dir):
    """Explicitly lists out every test binary in //tests."""
    all_tests = (
        run_bazel(
            (
                "query",
                "kind(cc_binary, //test/...)",
                "--output=label",
                override_picotool_arg(picotool_dir) if picotool_dir else "",
            ),
            text=True,
            check=True,
            capture_output=True,
        )
        .stdout.strip()
        .splitlines()
    )
    # Some tests are behind a transition.
    return [t.replace("_actual", "") for t in all_tests]


def build_all_configurations(picotool_dir):
    default_build_targets = [
        "//...",
        # Tests are explicitly built by name so we can catch compatibilty
        # regressions that cause them to vanish from the wildcard build.
        *_find_tests(picotool_dir),
    ]
    failed_builds = []
    for config in BUILD_CONFIGURATIONS:
        print_framed_string(f"Building {config['name']} configuration...")
        build_targets = [
            t for t in default_build_targets if t not in config["exclusions"]
        ]
        build_targets.extend(config["extra_targets"])

        args = list(config["args"])
        if picotool_dir:
            args.append(override_picotool_arg(picotool_dir))

        result = run_bazel(
            (
                "build",
                *args,
                *build_targets,
            ),
        )
        if result.returncode != 0:
            failed_builds.append(config["name"])
        print_to_stderr()

    if failed_builds:
        print_framed_string("ERROR: One or more builds failed.")
        for build in failed_builds:
            print_to_stderr(f"  * FAILED: {build} build")
        return 1

    print_framed_string("All builds successfully passed!")
    return 0


def main():
    setup_logging()
    args = parse_common_args()
    return build_all_configurations(args.picotool_dir)


if __name__ == "__main__":
    sys.exit(main())
