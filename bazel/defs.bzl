def _pico_generate_pio_header_impl(ctx):
    generated_headers = []
    for f in ctx.files.srcs:
        out = ctx.actions.declare_file(
            "{}_pio_generated/{}.h".format(ctx.label.name, f.basename)
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
        DefaultInfo(files=depset(direct = generated_headers)),
        CcInfo(compilation_context = cc_ctx),
    ]

pico_generate_pio_header = rule(
    implementation = _pico_generate_pio_header_impl,
    attrs = {
        "srcs": attr.label_list(mandatory = True, allow_files = True),
        "_pioasm_tool": attr.label(
            default = "@pico-sdk//tools/pioasm:pioasm",
            cfg = "exec",
            executable = True
        ),
    },
    provides = [CcInfo],
)
