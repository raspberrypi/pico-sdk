load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _pico_sdk_define_impl(ctx):
    val = ctx.attr.from_flag[BuildSettingInfo].value

    if type(val) == "string":
        # Strings need quotes.
        val = "\"{}\"".format(val)
    elif type(val) == "bool":
        # Convert bools to 0 or 1.
        val = 1 if val else 0
    cc_ctx = cc_common.create_compilation_context(
        defines = depset(
            direct = ["{}={}".format(ctx.attr.define_name, val)],
        ),
    )
    return [CcInfo(compilation_context = cc_ctx)]

pico_sdk_define = rule(
    implementation = _pico_sdk_define_impl,
    doc = """A simple rule that offers a skylib flag as a define.

These can be listed in the `deps` attribute of a `cc_library` to get access
to the value of a define.

Example:

    bool_flag(
        name = "my_flag",
        build_setting_default = False,
    )

    pico_sdk_define(
        name = "flag_define",
        define_name = "MY_FLAG_DEFINE",
        from_flag = ":my_flag",
    )
""",
    attrs = {
        "define_name": attr.string(mandatory = True),
        "from_flag": attr.label(mandatory = True),
    },
)
