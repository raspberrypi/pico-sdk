load("@bazel_skylib//rules/directory:directory.bzl", "directory")
load("@rules_cc//cc/toolchains:tool.bzl", "cc_tool")
load("@rules_cc//cc/toolchains:tool_map.bzl", "cc_tool_map")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

# Directory-based rules in this toolchain only referece things in
# lib/ or include/ subdirectories.
directory(
    name = "toolchain_root",
    srcs = glob([
        "lib/**",
        "include/**",
    ]),
)

cc_tool_map(
    name = "all_tools",
    tools = {
        "@rules_cc//cc/toolchains/actions:assembly_actions": ":asm",
        "@rules_cc//cc/toolchains/actions:c_compile": ":clang",
        "@rules_cc//cc/toolchains/actions:cpp_compile_actions": ":clang++",
        "@rules_cc//cc/toolchains/actions:link_actions": ":lld",
        "@rules_cc//cc/toolchains/actions:objcopy_embed_data": ":llvm-objcopy",
        "@rules_cc//cc/toolchains/actions:strip": ":llvm-strip",
        "@rules_cc//cc/toolchains/actions:ar_actions": ":llvm-ar",
    },
)

# TODO: https://github.com/bazelbuild/rules_cc/issues/235 - Workaround until
# Bazel has a more robust way to implement `cc_tool_map`.
alias(
    name = "asm",
    actual = ":clang",
)

cc_tool(
    name = "clang",
    src = select({
        "@platforms//os:windows": "//:bin/clang.exe",
        "//conditions:default": "//:bin/clang",
    }),
    data = ["bin/llvm"] + glob([
        "lib/clang/*/include/**",
        "include/armv*-unknown-none-eabi/**",
    ]),
)

cc_tool(
    name = "clang++",
    src = select({
        "@platforms//os:windows": "//:bin/clang++.exe",
        "//conditions:default": "//:bin/clang++",
    }),
    data = ["bin/llvm"] + glob([
        "lib/clang/*/include/**",
        "include/armv*-unknown-none-eabi/**",
        "include/c++/v1/**",
    ]),
)

cc_tool(
    name = "lld",
    src = select({
        "@platforms//os:windows": "//:bin/clang++.exe",
        "//conditions:default": "//:bin/clang++",
    }),
    data = ["bin/llvm"] + glob([
        "bin/lld*",
        "bin/ld*",
        "lib/**/*.a",
        "lib/**/*.so*",
        "lib/**/*.o",
        "lib/armv*-unknown-none-eabi/**",
        "lib/clang/*/lib/armv*-unknown-none-eabi/**",
    ]),
)

cc_tool(
    name = "llvm-ar",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-ar.exe",
        "//conditions:default": "//:bin/llvm-ar",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "llvm-libtool-darwin",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-libtool-darwin.exe",
        "//conditions:default": "//:bin/llvm-libtool-darwin",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "llvm-objcopy",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-objcopy.exe",
        "//conditions:default": "//:bin/llvm-objcopy",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "llvm-objdump",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-objdump.exe",
        "//conditions:default": "//:bin/llvm-objdump",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "llvm-cov",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-cov.exe",
        "//conditions:default": "//:bin/llvm-cov",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "llvm-strip",
    src = select({
        "@platforms//os:windows": "//:bin/llvm-strip.exe",
        "//conditions:default": "//:bin/llvm-strip",
    }),
    data = ["bin/llvm"],
)

cc_tool(
    name = "clang-tidy",
    src = select({
        "@platforms//os:windows": "//:bin/clang-tidy.exe",
        "//conditions:default": "//:bin/clang-tidy",
    }),
    data = ["bin/llvm"] + glob([
        "include/**",
        "lib/clang/**/include/**",
    ]),
)
