"""Rules for linker scripts."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _linker_scripts_impl(ctx):
    link_flags = []
    for script in ctx.attr.include_scripts:
        link_include_dir = script.label.package
        if ctx.label.workspace_root:
            link_include_dir = "/".join((ctx.label.workspace_root, link_include_dir))
        link_flag = "-L" + str(link_include_dir)
        if not (link_flag in link_flags):
            link_flags.append(link_flag)

    for script in ctx.files.link_scripts:
        link_flags.append("-T" + str(script.path))

    all_scripts = ctx.files.link_scripts + ctx.files.include_scripts

    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = link_flags,
        ),
        additional_inputs = depset(direct = all_scripts),
    )
    return [
        DefaultInfo(files = depset(direct = all_scripts)),
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

linker_scripts = rule(
    implementation = _linker_scripts_impl,
    attrs = {
        "link_scripts": attr.label_list(allow_files = [".ld"], doc = "List of scripts to explicitly link"),
        "include_scripts": attr.label_list(allow_files = [".incl"], doc = "List of scripts to include"),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
