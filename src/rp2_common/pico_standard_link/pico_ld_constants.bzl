load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")

def _generated_pico_ld_constants_impl(ctx):
    ld_constants_linker_fragment = ctx.actions.declare_file(ctx.label.name + "/ldinclude/pico_ld_constants.ld")
    link_include_dir = ld_constants_linker_fragment.dirname

    file_contents = "\n"
    ctx.actions.write(ld_constants_linker_fragment, file_contents)
    linking_inputs = cc_common.create_linker_input(
        owner = ctx.label,
        user_link_flags = depset(
            direct = ["-L" + str(link_include_dir)],
        ),
        additional_inputs = depset(
            direct = [ld_constants_linker_fragment],
        ),
    )
    return [
        DefaultInfo(files = depset([ld_constants_linker_fragment])),
        CcInfo(linking_context = cc_common.create_linking_context(linker_inputs = depset(direct = [linking_inputs]))),
    ]

generated_pico_ld_constants = rule(
    implementation = _generated_pico_ld_constants_impl,
    toolchains = use_cpp_toolchain(),
    fragments = ["cpp"],
)
