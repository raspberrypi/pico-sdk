"""Rules to create configurable toolchain features."""

load("@rules_cc//cc/toolchains:args.bzl", "cc_args")
load("@rules_cc//cc/toolchains:args_list.bzl", "cc_args_list")
load("@rules_cc//cc/toolchains:feature.bzl", "cc_feature")

def configurable_toolchain_feature(name, copts = None, cxxopts = None, linkopts = None):
    """Creates a configurable toolchain feature.

    Configurable toolchain features are features that can be conditionally
    enabled or disabled on a toolchain.

    Args:
        name: The name of the feature.
        copts: C compiler flags.
        cxxopts: C++ compiler flags.
        linkopts: Linker flags.
    """
    all_args = []
    copts = copts or []
    cxxopts = cxxopts or []
    linkopts = linkopts or []

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
    )
