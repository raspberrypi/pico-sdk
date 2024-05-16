load("@rules_cc//cc/toolchains:toolchain.bzl", "cc_toolchain")

HOSTS = (
    ("linux", "x86_64"),
    ("win", "x86_64"),
    ("mac", "x86_64"),
    ("mac", "aarch64"),
)

_HOST_OS_CONSTRAINTS = {
    "linux": "@platforms//os:linux",
    "win": "@platforms//os:windows",
    "mac": "@platforms//os:macos",
}

_HOST_CPU_CONSTRAINTS = {
    "x86_64": "@platforms//cpu:x86_64",
    "aarch64": "@platforms//cpu:aarch64",
}

def generate_toolchains():
    for host_os, host_cpu in HOSTS:
        _HOST_STR = "{}-{}".format(host_os, host_cpu)

        cc_toolchain(
            name = "arm_gcc_{}_toolchain_cortex-m".format(_HOST_STR),
            compiler = "gcc",  # Useful for distinguishing gcc vs clang.
            args = ["@pico-sdk//bazel/toolchain:all_unconditional_args"],
            toolchain_features = [
                "@pico-sdk//bazel/toolchain:legacy_features",
                "@pico-sdk//bazel/toolchain:override_debug",
            ],
            action_type_configs = [
                "@arm_gcc_{}//:arm-none-eabi-ar".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-gcc".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-g++".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-ld".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-objcopy".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-strip".format(_HOST_STR),
            ],
            sysroot = "external/arm_gcc_{}".format(_HOST_STR),
            cxx_builtin_include_directories = [
                "%sysroot%/arm-none-eabi/include/newlib-nano",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1/arm-none-eabi",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1/backward",
                "%sysroot%/lib/gcc/arm-none-eabi/13.2.1/include",
                "%sysroot%/lib/gcc/arm-none-eabi/13.2.1/include-fixed",
                "%sysroot%/arm-none-eabi/include",
            ],
            target_compatible_with = [
                "@pico-sdk//bazel/constraint:rp2040",
            ],
            exec_compatible_with = [
                _HOST_CPU_CONSTRAINTS[host_cpu],
                _HOST_OS_CONSTRAINTS[host_os],
            ],
            tags = ["manual"],  # Don't try to build this in wildcard builds.
        )

        native.toolchain(
            name = "arm_gcc_{}".format(_HOST_STR),
            target_compatible_with = [
                "@pico-sdk//bazel/constraint:rp2040",
            ],
            exec_compatible_with = [
                _HOST_CPU_CONSTRAINTS[host_cpu],
                _HOST_OS_CONSTRAINTS[host_os],
            ],
            toolchain = ":arm_gcc_{}_toolchain_cortex-m".format(_HOST_STR),
            toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
        )
