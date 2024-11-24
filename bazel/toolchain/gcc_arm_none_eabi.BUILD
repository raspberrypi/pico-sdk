load("@bazel_skylib//rules/directory:directory.bzl", "directory")
load("@bazel_skylib//rules/directory:subdirectory.bzl", "subdirectory")
load("@rules_cc//cc/toolchains:tool.bzl", "cc_tool")
load("@rules_cc//cc/toolchains:tool_map.bzl", "cc_tool_map")
load("@rules_cc//cc/toolchains:args.bzl", "cc_args")
load("@rules_cc//cc/toolchains:args_list.bzl", "cc_args_list")


package(default_visibility = ["//visibility:public"])

cc_tool_map(
    name = "all_tools",
    tools = {
        "@rules_cc//cc/toolchains/actions:assembly_actions": ":asm",
        "@rules_cc//cc/toolchains/actions:c_compile": ":arm-none-eabi-gcc",
        "@rules_cc//cc/toolchains/actions:cpp_compile_actions": ":arm-none-eabi-g++",
        "@rules_cc//cc/toolchains/actions:link_actions": ":arm-none-eabi-ld",
        "@rules_cc//cc/toolchains/actions:objcopy_embed_data": ":arm-none-eabi-objcopy",
        "@rules_cc//cc/toolchains/actions:strip": ":arm-none-eabi-strip",
        "@rules_cc//cc/toolchains/actions:ar_actions": ":arm-none-eabi-ar",
    },
)

# TODO: https://github.com/bazelbuild/rules_cc/issues/235 - Workaround until
# Bazel has a more robust way to implement `cc_tool_map`.
alias(
    name = "asm",
    actual = ":arm-none-eabi-gcc",
)

cc_tool(
    name = "arm-none-eabi-ar",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-ar.exe",
        "//conditions:default": "//:bin/arm-none-eabi-ar",
    }),
)

cc_tool(
    name = "arm-none-eabi-g++",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-g++.exe",
        "//conditions:default": "//:bin/arm-none-eabi-g++",
    }),
    data = glob([
        "**/*.spec",
        "**/*.specs",
        "arm-none-eabi/include/**",
        "lib/gcc/arm-none-eabi/*/include/**",
        "lib/gcc/arm-none-eabi/*/include-fixed/**",
        "libexec/**",
    ]),
)

cc_tool(
    name = "arm-none-eabi-gcc",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-gcc.exe",
        "//conditions:default": "//:bin/arm-none-eabi-gcc",
    }),
    data = glob([
        "**/*.spec",
        "**/*.specs",
        "arm-none-eabi/include/**",
        "lib/gcc/arm-none-eabi/*/include/**",
        "lib/gcc/arm-none-eabi/*/include-fixed/**",
        "libexec/**",
    ]) +
    # The assembler needs to be explicitly added. Note that the path is
    # intentionally different here as `as` is called from arm-none-eabi-gcc.
    # `arm-none-eabi-as` will not suffice for this context.
    select({
        "@platforms//os:windows": ["//:arm-none-eabi/bin/as.exe"],
        "//conditions:default": ["//:arm-none-eabi/bin/as"],
    }),
)

# This tool is actually just g++ under the hood, but this specifies a
# different set of data files to pull into the sandbox at runtime.
cc_tool(
    name = "arm-none-eabi-ld",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-g++.exe",
        "//conditions:default": "//:bin/arm-none-eabi-g++",
    }),
    data = glob([
        "**/*.a",
        "**/*.ld",
        "**/*.o",
        "**/*.spec",
        "**/*.specs",
        "**/*.so",
        "libexec/**",
    ]),
)

cc_tool(
    name = "arm-none-eabi-objcopy",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-objcopy.exe",
        "//conditions:default": "//:bin/arm-none-eabi-objcopy",
    }),
)

cc_tool(
    name = "arm-none-eabi-strip",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-strip.exe",
        "//conditions:default": "//:bin/arm-none-eabi-strip",
    }),
)

cc_tool(
    name = "arm-none-eabi-objdump",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-objdump.exe",
        "//conditions:default": "//:bin/arm-none-eabi-objdump",
    }),
)

# There is not yet a well-known action type for objdump.

cc_tool(
    name = "arm-none-eabi-gcov",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-gcov.exe",
        "//conditions:default": "//:bin/arm-none-eabi-gcov",
    }),
)

# There is not yet a well-known action type for gcov.
