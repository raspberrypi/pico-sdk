load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")

def _include_linker_script_dir_impl(ctx):
    link_include_dir = str(ctx.label.package)
    depset_direct = ["-L" + str(link_include_dir)]
    if len(ctx.files.use_scripts):
        for script in ctx.files.use_scripts:
            depset_direct.append("-T" + str(script.path))

    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = depset_direct,
        ),
    )
    return [
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

include_linker_script_dir = rule(
    implementation = _include_linker_script_dir_impl,
    attrs = {
        "use_scripts": attr.label_list(allow_files = [".ld"]),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)

def _use_linker_script_file_impl(ctx):
    link_file = ctx.file.script.path

    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = ["-T" + str(link_file)],
        ),
    )
    return [
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

use_linker_script_file = rule(
    implementation = _use_linker_script_file_impl,
    attrs = {
        "script": attr.label(mandatory = True, allow_single_file = [".ld"]),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
