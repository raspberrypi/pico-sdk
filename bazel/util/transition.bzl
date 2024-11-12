def _normalize_flag_value(val):
    """Converts flag values to transition-safe primitives."""
    if type(val) == "label":
        return str(val)
    return val

def declare_transtion(attrs, flag_overrides = None, append_to_flags = None, executable = True):
    """A helper that drastically simplifies declaration of a transition.

    A transition in Bazel is a way to force changes to the way the build is
    evaluated for all dependencies of a given rule.

    Imagine the following simple dependency graph:

        ->: depends on
        a -> b -> c

    Normally, if you set `defines` on a, they couldn't apply to b or c because
    they are dependencies of a. There's no way for b or c to know about a's
    settings, because they don't even know a exists!

    We can fix this via a transition! If we put a transition in front of `a`
    that sets --copts=-DFOO=42, we're telling Bazel to build a and all of its
    dependencies under that configuration.

    Note: Flags must be referenced as e.g. `//command_line_option:copt` in
    transitions.

    `declare_transition()` eliminates the frustrating amount of boilerplate. All
    you need to do is provide a set of attrs, and then a `flag_overrides`
    dictionary that tells `declare_transition()` which attrs to pull flag values
    from. The common `src` attr tells the transition which build rule to apply
    the transition to.
    """

    def _flag_override_impl(settings, attrs):
        final_overrides = {}
        if flag_overrides != None:
            final_overrides = {
                key: _normalize_flag_value(getattr(attrs, value))
                for key, value in flag_overrides.items()
            }
        if append_to_flags != None:
            for flag, field in append_to_flags.items():
                accumulated_flags = final_overrides.get(flag, settings.get(flag, []))
                accumulated_flags.extend(
                    [str(val) for val in getattr(attrs, field)],
                )
                final_overrides[flag] = accumulated_flags
        return final_overrides

    output_flags = []
    if flag_overrides != None:
        output_flags.extend(flag_overrides.keys())
    if append_to_flags != None:
        output_flags.extend(append_to_flags.keys())
    _transition = transition(
        implementation = _flag_override_impl,
        inputs = append_to_flags.keys() if append_to_flags != None else [],
        outputs = output_flags,
    )

    def _symlink_artifact_impl(ctx):
        out = ctx.actions.declare_file(ctx.label.name)
        if executable:
            ctx.actions.symlink(output = out, target_file = ctx.executable.src)
            return [DefaultInfo(files = depset([out]), executable = out)]

        ctx.actions.symlink(
            output = out,
            target_file = ctx.attr.src[0][DefaultInfo].files.to_list()[0],
        )
        return [DefaultInfo(files = depset([out]))]

    return rule(
        implementation = _symlink_artifact_impl,
        executable = executable,
        attrs = {
            "src": attr.label(
                cfg = _transition,
                executable = executable,
                mandatory = True,
            ),
        } | attrs,
    )

# This transition is applied before building the boot_stage2 image.
rp2040_bootloader_binary = declare_transtion(
    attrs = {
        "_malloc": attr.label(default = "//bazel:empty_cc_lib"),
        # This could be shared, but we don't in order to make it clearer that
        # a transition is in use.
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
	"_link_extra_libs": attr.label(default = "//bazel:empty_cc_lib"),
    },
    flag_overrides = {
        # We don't want --custom_malloc to ever apply to the bootloader, so
        # always explicitly override it here.
        "//command_line_option:custom_malloc": "_malloc",

        # Platforms will commonly depend on bootloader components in every
        # binary via `link_extra_libs`, so we must drop these deps when
        # building the bootloader binaries themselves in order to avoid a
        # circular dependency.
	"@bazel_tools//tools/cpp:link_extra_libs": "_link_extra_libs",
    },
)

# This transition sets SDK configuration options required to build test binaries
# for the kitchen_sink suite of tests.
kitchen_sink_test_binary = declare_transtion(
    attrs = {
        "bt_stack_config": attr.label(mandatory = True),
        "lwip_config": attr.label(mandatory = True),
        "enable_ble": attr.bool(default = False),
        "enable_bt_classic": attr.bool(default = False),
        # This could be shared, but we don't in order to make it clearer that
        # a transition is in use.
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    flag_overrides = {
        "@pico-sdk//bazel/config:PICO_BTSTACK_CONFIG": "bt_stack_config",
        "@pico-sdk//bazel/config:PICO_LWIP_CONFIG": "lwip_config",
        "@pico-sdk//bazel/config:PICO_BT_ENABLE_BLE": "enable_ble",
        "@pico-sdk//bazel/config:PICO_BT_ENABLE_CLASSIC": "enable_bt_classic",
    },
)

# This transition sets SDK configuration options required to build test binaries
# for the pico_float_test suite of tests.
pico_float_test_binary = declare_transtion(
    attrs = {
        "pico_printf_impl": attr.string(),
        "extra_copts": attr.string_list(),
        # This could be shared, but we don't in order to make it clearer that
        # a transition is in use.
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    flag_overrides = {
        "@pico-sdk//bazel/config:PICO_DEFAULT_PRINTF_IMPL": "pico_printf_impl",
    },
    append_to_flags = {
        "//command_line_option:copt": "extra_copts",
    },
)

# This is a general purpose transition that applies the listed copt flags to
# all transitive dependencies.
extra_copts_for_all_deps = declare_transtion(
    attrs = {
        "extra_copts": attr.string_list(),
        # This could be shared, but we don't in order to make it clearer that
        # a transition is in use.
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
    append_to_flags = {
        "//command_line_option:copt": "extra_copts",
    },
)
