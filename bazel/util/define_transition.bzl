def _add_defines_to_current_cfg():
    def _build_with_overridden_defines_impl(settings, attr):
        copts = settings["//command_line_option:copt"]
        copts += ["-D{}".format(define) for define in attr.defines]
        return {
            "//command_line_option:copt": copts,
        }

    return transition(
        implementation = _build_with_overridden_defines_impl,
        inputs = ["//command_line_option:copt"],
        outputs = ["//command_line_option:copt"],
    )

_transition = _add_defines_to_current_cfg()

def _binary_with_overridden_defines_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.symlink(output = out, target_file = ctx.executable.binary)
    return [DefaultInfo(files = depset([out]), executable = out)]

binary_with_overridden_defines = rule(
    implementation = _binary_with_overridden_defines_impl,
    executable = True,
    attrs = {
        "binary": attr.label(
            cfg = _transition,
            executable = True,
            mandatory = True,
        ),
        "defines": attr.string_list(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
