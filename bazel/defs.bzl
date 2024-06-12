def _pico_generate_pio_header_impl(ctx):
    generated_headers = []
    for f in ctx.files.srcs:
        out = ctx.actions.declare_file(
            "{}_pio_generated/{}.h".format(ctx.label.name, f.basename),
        )
        generated_headers.append(out)
        ctx.actions.run(
            executable = ctx.executable._pioasm_tool,
            arguments = [
                "-o",
                "c-sdk",
                f.path,
                out.path,
            ],
            inputs = [f],
            outputs = [out],
        )

    cc_ctx = cc_common.create_compilation_context(
        headers = depset(direct = generated_headers),
        includes = depset(direct = [generated_headers[0].dirname]),
    )
    return [
        DefaultInfo(files = depset(direct = generated_headers)),
        CcInfo(compilation_context = cc_ctx),
    ]

pico_generate_pio_header = rule(
    implementation = _pico_generate_pio_header_impl,
    attrs = {
        "srcs": attr.label_list(mandatory = True, allow_files = True),
        "_pioasm_tool": attr.label(
            default = "@pico-sdk//tools/pioasm:pioasm",
            cfg = "exec",
            executable = True,
        ),
    },
    provides = [CcInfo],
)

# Because the syntax for target_compatible_with when used with config_setting
# rules is both confusing and verbose, provide some helpers that make it much
# easier and clearer to express compatibility.
#
# Context: https://github.com/bazelbuild/bazel/issues/12614

def compatible_with_config(config_label):
    """Expresses compatibility with a config_setting."""
    return select({
        config_label: [],
        "//conditions:default": ["@platforms//:incompatible"],
    })

def incompatible_with_config(config_label):
    """Expresses incompatibility with a config_setting."""
    return select({
        config_label: ["@platforms//:incompatible"],
        "//conditions:default": [],
    })

def compatible_with_rp2():
    """Expresses a rule is compatible with the rp2 family."""
    return incompatible_with_config("//bazel/constraint:host")

def compatible_with_pico_w():
    """Expresses a rule is compatible a Pico W."""
    return select({
        "@pico-sdk//bazel/constraint:cyw43_wireless": [],
        "@pico-sdk//bazel/constraint:is_pico_w": [],
        "//conditions:default": ["@platforms//:incompatible"],
    })
