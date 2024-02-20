load(
    "@pw_toolchain//cc_toolchain:defs.bzl",
    "pw_cc_flag_set",
    "pw_cc_toolchain",
)

HOSTS = (
    ("win", "x86_64"),
    ("mac", "x86_64"),
    ("mac", "aarch64"),
)

_HOST_OS_CONSTRAINTS = {
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

        pw_cc_toolchain(
            name = "arm_gcc_{}_toolchain_cortex-m".format(_HOST_STR),
            action_config_flag_sets = [
                "@pw_toolchain//flag_sets:o2",
                "@pw_toolchain//flag_sets:c++17",
                "@pw_toolchain//flag_sets:debugging",
                "@pw_toolchain//flag_sets:reduced_size",
                "@pw_toolchain//flag_sets:no_canonical_prefixes",
                "@pw_toolchain//flag_sets:no_rtti",
                "@pw_toolchain//flag_sets:wno_register",
                "@pw_toolchain//flag_sets:wnon_virtual_dtor",
                "@pico-sdk//bazel/toolchain:cortex-m0",
                "@pico-sdk//bazel/toolchain:thumb_abi",
                "@pico-sdk//bazel/toolchain:cortex_common",
                "@pico-sdk//bazel/toolchain:cortex_common_link",
                "@pico-sdk//bazel/toolchain:warnings",
                "@pico-sdk//bazel/toolchain:pico_sdk_defines",
            ],
            action_configs = [
                "@arm_gcc_{}//:arm-none-eabi-ar".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-gcc".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-g++".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-gcov".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-ld".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-objcopy".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-objdump".format(_HOST_STR),
                "@arm_gcc_{}//:arm-none-eabi-strip".format(_HOST_STR),
            ],
            builtin_sysroot = "external/arm_gcc_{}".format(_HOST_STR),
            cxx_builtin_include_directories = [
                "%sysroot%/arm-none-eabi/include/newlib-nano",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1/arm-none-eabi",
                "%sysroot%/arm-none-eabi/include/c++/13.2.1/backward",
                "%sysroot%/lib/gcc/arm-none-eabi/13.2.1/include",
                "%sysroot%/lib/gcc/arm-none-eabi/13.2.1/include-fixed",
                "%sysroot%/arm-none-eabi/include",
            ],
            target_compatible_with = select({
                "@pw_toolchain//constraints/arm_mcpu:cortex-m0": [],
                "@pw_toolchain//constraints/arm_mcpu:none": ["@platforms//:incompatible"],
            }),
            toolchain_identifier = "arm-gcc-toolchain",
            # make_variables = {
            #     "OBJCOPY": "arm-none-eabi-objcopy",
            # },
        )

        native.toolchain(
            name = "arm_gcc_{}".format(_HOST_STR),
            target_compatible_with = [
                "@pw_toolchain//constraints/arm_mcpu:cortex-m0",
            ],
            exec_compatible_with = [
                _HOST_CPU_CONSTRAINTS[host_cpu],
                _HOST_OS_CONSTRAINTS[host_os],
            ],
            toolchain = ":arm_gcc_{}_toolchain_cortex-m".format(_HOST_STR),
            toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
        )
