load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")

def _cc_toolchain_feature_is_enabled_impl(ctx):
    toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = toolchain,
    )
    val = cc_common.is_enabled(
        feature_configuration = feature_configuration,
        feature_name = ctx.attr.feature_name,
    )

def _match_label_flag_impl(ctx):
    matches = str(ctx.attr.expected_value.label) == str(ctx.attr.flag.label)
    return [
        config_common.FeatureFlagInfo(value = str(matches)),
        BuildSettingInfo(value = matches),
    ]

_match_label_flag = rule(
    implementation = _match_label_flag_impl,
    attrs = {
        "expected_value": attr.label(
            mandatory = True,
            doc = "The expected flag value",
        ),
        "flag": attr.label(
            mandatory = True,
            doc = "The flag to extract a value from",
        ),
    },
)

def label_flag_matches(*, name, flag, value):
    _match_label_flag(
        name = name + "._impl",
        expected_value = native.package_relative_label(value),
        flag = flag,
    )

    native.config_setting(
        name = name,
        flag_values = {":{}".format(name + "._impl"): "True"},
    )
