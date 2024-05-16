load("@rules_cc//cc/toolchains:action_type_config.bzl", "cc_action_type_config")
load("@rules_cc//cc/toolchains:tool.bzl", "cc_tool")

package(default_visibility = ["//visibility:public"])

cc_tool(
    name = "arm-none-eabi-ar_tool",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-ar.exe",
        "//conditions:default": "//:bin/arm-none-eabi-ar",
    }),
)

cc_action_type_config(
    name = "arm-none-eabi-ar",
    action_types = ["@rules_cc//cc/toolchains/actions:ar_actions"],
    tools = [":arm-none-eabi-ar_tool"],
)

cc_tool(
    name = "arm-none-eabi-g++_tool",
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

cc_action_type_config(
    name = "arm-none-eabi-g++",
    action_types = ["@rules_cc//cc/toolchains/actions:cpp_compile_actions"],
    tools = [":arm-none-eabi-g++_tool"],
)

cc_tool(
    name = "arm-none-eabi-gcc_tool",
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

cc_action_type_config(
    name = "arm-none-eabi-gcc",
    action_types = [
        "@rules_cc//cc/toolchains/actions:assembly_actions",
        "@rules_cc//cc/toolchains/actions:c_compile",
    ],
    tools = [":arm-none-eabi-gcc_tool"],
)

# This tool is actually just g++ under the hood, but this specifies a
# different set of data files to pull into the sandbox at runtime.
cc_tool(
    name = "arm-none-eabi-ld_tool",
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

cc_action_type_config(
    name = "arm-none-eabi-ld",
    action_types = ["@rules_cc//cc/toolchains/actions:link_actions"],
    tools = [":arm-none-eabi-ld_tool"],
)

cc_tool(
    name = "arm-none-eabi-objcopy_tool",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-objcopy.exe",
        "//conditions:default": "//:bin/arm-none-eabi-objcopy",
    }),
)

cc_action_type_config(
    name = "arm-none-eabi-objcopy",
    action_types = ["@rules_cc//cc/toolchains/actions:objcopy_embed_data"],
    tools = [":arm-none-eabi-objcopy_tool"],
)

cc_tool(
    name = "arm-none-eabi-strip_tool",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-strip.exe",
        "//conditions:default": "//:bin/arm-none-eabi-strip",
    }),
)

cc_action_type_config(
    name = "arm-none-eabi-strip",
    action_types = ["@rules_cc//cc/toolchains/actions:strip"],
    tools = [":arm-none-eabi-strip_tool"],
)

cc_tool(
    name = "arm-none-eabi-objdump_tool",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-objdump.exe",
        "//conditions:default": "//:bin/arm-none-eabi-objdump",
    }),
)

# There is not yet a well-known action type for objdump.

cc_tool(
    name = "arm-none-eabi-gcov_tool",
    src = select({
        "@platforms//os:windows": "//:bin/arm-none-eabi-gcov.exe",
        "//conditions:default": "//:bin/arm-none-eabi-gcov",
    }),
)

# There is not yet a well-known action type for gcov.
