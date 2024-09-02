load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")

def _generated_pico_flash_region_impl(ctx):
    flash_region_linker_fragment = ctx.actions.declare_file(ctx.label.name + "/ldinclude/pico_flash_region.ld")
    link_include_dir = flash_region_linker_fragment.dirname

    file_contents = "\n".join((
        "FLASH(rx) : ORIGIN = 0x10000000, LENGTH = " + str(ctx.attr.flash_region_size),
    ))
    ctx.actions.write(flash_region_linker_fragment, file_contents)
    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = ["-L" + str(link_include_dir)],
        ),
        additional_inputs = depset(
            direct = [flash_region_linker_fragment],
        ),
    )
    return [
        DefaultInfo(files = depset([flash_region_linker_fragment])),
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

generated_pico_flash_region = rule(
    implementation = _generated_pico_flash_region_impl,
    attrs = {
        "flash_region_size": attr.int(mandatory = True),
    },
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
