load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cpp_toolchain", "use_cc_toolchain")

def _pico_btstack_make_gatt_header_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    out = ctx.actions.declare_file(
        "{}_gatt_generated/{}.h".format(ctx.label.name, ctx.file.src.basename.removesuffix(".gatt")),
    )

    ctx.actions.run(
        executable = ctx.executable._make_gat_header_tool,
        arguments = [
            ctx.file.src.path,
            out.path,
            "-I",
            ctx.file._btstack_hdr.dirname,
        ] + [

        ],
        inputs = [
            ctx.file.src,
            ctx.file._btstack_hdr,
        ],
        outputs = [out],
    )

    cc_ctx = cc_common.create_compilation_context(
        headers = depset(direct = [out]),
        includes = depset(direct = [out.dirname]),
    )

    return [
        DefaultInfo(files = depset(direct = [out])),
        CcInfo(compilation_context = cc_ctx)
    ]

pico_btstack_make_gatt_header = rule(
    implementation = _pico_btstack_make_gatt_header_impl,
    attrs = {
        "src": attr.label(mandatory = True, allow_single_file = True),
        "_btstack_hdr": attr.label(
            default = "@btstack//:src/bluetooth_gatt.h",
            allow_single_file = True,
        ),
        "_make_gat_header_tool": attr.label(
            default = "@btstack//:compile_gatt",
            cfg = "exec",
            executable = True,
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain(),
)
