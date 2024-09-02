load("@rules_cc//cc/toolchains:args.bzl", "cc_args")
load("@rules_cc//cc/toolchains:args_list.bzl", "cc_args_list")
load("@rules_cc//cc/toolchains:feature.bzl", "cc_feature")

def configurable_toolchain_feature(name, copts = [], cxxopts = [], linkopts = [], enable_if = None, disable_if = None):
    if enable_if != None and disable_if != None:
        fail("Cannot specify both enable_if and disable_if")
    if enable_if == None and disable_if == None:
        fail("Must specify at least one of enable_if and disable_if")
    if enable_if == None:
        enable_if = "//conditions:default"
    if disable_if == None:
        disable_if = "//conditions:default"

    all_args = []

    if copts:
        cc_args(
            name = name + "_cc_args",
            actions = ["@rules_cc//cc/toolchains/actions:compile_actions"],
            args = copts,
        )
        all_args.append(name + "_cc_args")

    if cxxopts:
        cc_args(
            name = name + "_cxx_args",
            actions = ["@rules_cc//cc/toolchains/actions:cpp_compile_actions"],
            args = cxxopts,
        )
        all_args.append(name + "_cxx_args")

    if linkopts:
        cc_args(
            name = name + "_link_args",
            actions = ["@rules_cc//cc/toolchains/actions:link_actions"],
            args = linkopts,
        )
        all_args.append(name + "_link_args")

    cc_args_list(
        name = name + "_args",
        args = all_args,
    )

    cc_feature(
        name = name,
        feature_name = name,
        args = [":{}_args".format(name)],
        enabled = select({
            disable_if: False,
            enable_if: True,
        }),
    )
