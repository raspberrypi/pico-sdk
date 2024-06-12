# A transition in Bazel is a way to force changes to the way the build is
# evaluated for all dependencies of a given rule.
#
# Imagine the following simple dependency graph:
#
#     ->: depends on
#     a -> b -> c
#
# Normally, if you set `defines` on a, they couldn't apply to b or c because
# they are dependencies of a. There's no way for b or c to know about a's
# settings, because they don't even know a exists!
#
# We can fix this via a transition! If we put a transition in front of `a`
# that sets --copts=-DFOO=42, we're telling Bazel to build a and all of its
# dependencies under that configuration.
#
# Note: Flags must be referenced as e.g. `//command_line_option:copt` in
# transitions.
#
# `declare_transition()` eliminates the frustrating amount of boilerplate. All
# you need to do is provide a set of attrs, and then a `flag_overrides`
# dictionary that tells `declare_transition()` which attrs to pull flag values
# from. The common `src` attr tells the transition which build rule to apply
# the transition to.
def declare_transtion(attrs, flag_overrides, executable = True):
    def _flag_override_impl(settings, attrs):
        return {
            key: str(getattr(attrs, value))
            for key, value in flag_overrides.items()
        }

    _transition = transition(
        implementation = _flag_override_impl,
        inputs = [],
        outputs = flag_overrides.keys(),
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
            "_allowlist_function_transition": attr.label(
                default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
            ),
        } | attrs,
    )

rp2040_bootloader_binary = declare_transtion(
    attrs = {
        "_malloc": attr.label(default = "//bazel:empty_cc_lib"),
    },
    flag_overrides = {
        # We don't want --custom_malloc to ever apply to the bootloader, so
        # always explicitly override it here.
        "//command_line_option:custom_malloc": "_malloc",
    },
)

kitchen_sink_test_binary =  declare_transtion(
    attrs = {
        "bt_stack_config": attr.label(mandatory = True),
        "lwip_config": attr.label(mandatory = True),
    },
    flag_overrides = {
        "@pico-sdk//bazel/config:PICO_BTSTACK_CONFIG": "bt_stack_config",
        "@pico-sdk//bazel/config:PICO_LWIP_CONFIG": "lwip_config",
    },
)
