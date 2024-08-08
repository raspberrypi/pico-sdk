load("@rules_cc//cc/toolchains:action_type_config.bzl", "cc_action_type_config")
load("@rules_cc//cc/toolchains:tool.bzl", "cc_tool")

package(default_visibility = ["//visibility:public"])

cc_tool(
    name = "llvm-ar_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-ar.exe",
        "//conditions:default": "//:bin/llvm-ar",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "llvm-ar",
    action_types = ["@rules_cc//cc/toolchains/actions:ar_actions"],
    tools = [":llvm-ar_tool"],
)

cc_tool(
    name = "clang_tool",
    src = select({
        "@platforms//os:windows": "//:bin/clang.exe",
        "//conditions:default": "//:bin/clang",
    }),
    data = glob([
        "include/armv*-unknown-none-eabi/**",
        "lib/clang/*/include/**",
    ]) + select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "clang",
    action_types = [
        "@rules_cc//cc/toolchains/actions:assembly_actions",
        "@rules_cc//cc/toolchains/actions:c_compile",
    ],
    tools = [":clang_tool"],
)

cc_tool(
    name = "clang++_tool",
    src = select({
        "@platforms//os:windows": "//:bin/clang++.exe",
        "//conditions:default": "//:bin/clang++",
    }),
    data = glob([
        "include/armv*-unknown-none-eabi/**",
        "include/c++/**",
        "lib/clang/*/include/**",
    ]) + select({
        # Windows doesn't have llvm.exe.
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "clang++",
    action_types = ["@rules_cc//cc/toolchains/actions:cpp_compile_actions"],
    tools = [":clang++_tool"],
)

# This tool is actually just clang++ under the hood, but this specifies a
# different set of data files to pull into the sandbox at runtime.
cc_tool(
    name = "lld_tool",
    src = select({
        "@platforms//os:windows": "//:bin/clang++.exe",
        "//conditions:default": "//:bin/clang++",
    }),
    data = glob([
        "lib/armv*-unknown-none-eabi/**",
        "lib/clang/*/lib/armv*-unknown-none-eabi/**",
    ]) + select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "lld",
    action_types = ["@rules_cc//cc/toolchains/actions:link_actions"],
    tools = [":lld_tool"],
)

cc_tool(
    name = "llvm-objcopy_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-objcopy.exe",
        "//conditions:default": "//:bin/llvm-objcopy",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "llvm-objcopy",
    action_types = ["@rules_cc//cc/toolchains/actions:objcopy_embed_data"],
    tools = [":llvm-objcopy_tool"],
)

cc_tool(
    name = "llvm-strip_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-strip.exe",
        "//conditions:default": "//:bin/llvm-strip",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

cc_action_type_config(
    name = "llvm-strip",
    action_types = ["@rules_cc//cc/toolchains/actions:strip"],
    tools = [":llvm-strip_tool"],
)

cc_tool(
    name = "llvm-objdump_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-objdump.exe",
        "//conditions:default": "//:bin/llvm-objdump",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

# There is not yet a well-known action type for llvm-objdump.

cc_tool(
    name = "llvm-profdata_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-profdata.exe",
        "//conditions:default": "//:bin/llvm-profdata",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

# There is not yet a well-known action type for llvm-profdata.

cc_tool(
    name = "llvm-cov_tool",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-cov.exe",
        "//conditions:default": "//:bin/llvm-cov",
    }),
    data = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["//:bin/llvm"],
    }),
)

# There is not yet a well-known action type for llvm-cov.
