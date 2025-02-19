load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@rules_cc//cc:defs.bzl", "cc_library")

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
    doc = """Generates a .h header file for each listed pio source.

Each source file listed in `srcs` will be available as `[pio file name].h` on
the include path if you depend on this rule from a `cc_library`.

pico_generate_pio_header(
    name = "my_fun_pio",
    srcs = ["my_fun_pio.pio"],
)

# This library can #include "my_fun_pio.pio.h".
cc_library(
    name = "libfoo",
    deps = [":my_fun_pio"],
    srcs = ["libfoo.c"],
)
""",
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
        "@pico-sdk//bazel/constraint:is_pico2_w": [],
        "//conditions:default": ["@platforms//:incompatible"],
    })

def pico_board_config(name, platform_includes, **kwargs):
    """A helper macro for declaring a Pico board to use with PICO_CONFIG_HEADER.

    This generates pico_config_platform_headers.h using the list of
    includes provided in `platform_includes`, and the final artifact is
    a cc_library that you can configure //bazel/config:PICO_CONFIG_HEADER to
    point to.
    """
    _hdr_dir = "{}_generated_includes".format(name)
    _hdr_path = "{}/pico_config_platform_headers.h".format(_hdr_dir)
    write_file(
        name = "{}_platform_headers_file".format(name),
        out = _hdr_path,
        content = ['#include "{}"'.format(inc) for inc in platform_includes],
    )
    kwargs.setdefault("hdrs", [])
    kwargs["hdrs"].append(_hdr_path)
    kwargs.setdefault("includes", [])
    kwargs["includes"].append(_hdr_dir)
    cc_library(
        name = name,
        **kwargs
    )
