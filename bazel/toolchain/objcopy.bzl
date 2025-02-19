load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "OBJ_COPY_ACTION_NAME")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cpp_toolchain", "use_cc_toolchain")

def _objcopy_to_bin_impl(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    objcopy_tool_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = OBJ_COPY_ACTION_NAME,
    )

    ctx.actions.run(
        inputs = depset(
            direct = [ctx.file.src],
            transitive = [cc_toolchain.all_files],
        ),
        executable = objcopy_tool_path,
        outputs = [ctx.outputs.out],
        arguments = [
            ctx.file.src.path,
            "-Obinary",
            ctx.outputs.out.path,
        ],
    )

objcopy_to_bin = rule(
    implementation = _objcopy_to_bin_impl,
    attrs = {
        "src": attr.label(
            allow_single_file = True,
            mandatory = True,
            doc = "File to use as input to objcopy command",
        ),
        "out": attr.output(
            mandatory = True,
            doc = "Destination file for objcopy command",
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain(),
)
