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
            direct = ["{}={}".format(ctx.attr.define_name, val)]
        )
    )
    return [CcInfo(compilation_context = cc_ctx)]

pico_sdk_define = rule(
    implementation = _pico_sdk_define_impl,
    attrs = {
        "define_name": attr.string(mandatory = True),
        "from_flag": attr.label(mandatory = True),
    }
)
