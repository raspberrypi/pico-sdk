# TODO: Default to a list of known compatible rules until the toolchain emits
# firmware images with a .elf extension. When binaries have a .elf suffix,
# this can change to ["*"] and another attribute that allows extension-based
# filtering can be added to more easily support a wider array of file types.
_SUPPORTED_BINARY_TYPES = ",".join([
    "cc_binary",
    "cc_test",
])

def _pico_uf2_aspect_impl(target, ctx):
    allowed_types = ctx.attr.from_rules.split(",")
    if ctx.rule.kind not in allowed_types and "*" not in allowed_types:
        return []

    binary_to_convert = target[DefaultInfo].files_to_run.executable
    uf2_output = ctx.actions.declare_file(binary_to_convert.basename + ".uf2")
    ctx.actions.run(
        outputs = [uf2_output],
        inputs = [binary_to_convert],
        tools = [ctx.executable._picotool],
        executable = ctx.executable._picotool,
        arguments = [
            "uf2",
            "convert",
            "--quiet",
            "-t",
            "elf",
            binary_to_convert.path,
            uf2_output.path,
        ],
    )
    return [
        OutputGroupInfo(
            pico_uf2_files = depset([uf2_output]),
        ),
    ]

pico_uf2_aspect = aspect(
    implementation = _pico_uf2_aspect_impl,
    doc = """An aspect for generating UF2 images from ELF binaries.

Normally with Bazel, a cc_binary or other rule cannot be "extended" to emit
additional outputs. However, this aspect may be used as a secondary, adjacent
step that generates UF2 images from all ELF artifacts.

This can be used from a build to produce UF2 files alongside the regular
outputs:

```
bazel build --platforms=@pico-sdk//bazel/platform:rp2040 \\
    --aspects @pico-sdk//tools:uf2_aspect.bzl%pico_uf2_aspect \\
    --output_groups=+pico_uf2_files \\
    //...
```

It's also possible to use this aspect within a custom macro (e.g. my_cc_binary)
to produce UF2 images alongside ELF files. However, with that method UF2 images
will only be produced when you explicitly use your custom macro.
""",
    attrs = {
        "from_rules": attr.string(
            default = _SUPPORTED_BINARY_TYPES,
            doc = "A comma-separated list of rule kinds to apply the UF2 aspect to",
        ),
        "_picotool": attr.label(default = "@picotool//:picotool", executable = True, cfg = "exec"),
    },
)
