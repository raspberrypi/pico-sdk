"""Utilities for creating psram regions."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _generated_pico_psram_region_impl(ctx):
    psram_region_linker_fragment = ctx.actions.declare_file(ctx.label.name + "/ldinclude/pico_psram_region.ld")
    link_include_dir = psram_region_linker_fragment.dirname

    file_contents = "\n".join((
        "PSRAM(rwx) : ORIGIN = 0x11000000, LENGTH = " + str(ctx.attr.psram_region_size),
    ))
    ctx.actions.write(psram_region_linker_fragment, file_contents)
    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = ["-L" + str(link_include_dir)],
        ),
        additional_inputs = depset(
            direct = [psram_region_linker_fragment],
        ),
    )
    return [
        DefaultInfo(files = depset([psram_region_linker_fragment])),
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

generated_pico_psram_region = rule(
    implementation = _generated_pico_psram_region_impl,
    attrs = {
        "psram_region_size": attr.int(mandatory = True),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
