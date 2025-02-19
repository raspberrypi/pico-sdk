load("@rules_cc//cc:defs.bzl", "cc_library")

# PICO_BUILD_DEFINE: PICO_PROGRAM_NAME, Provided by PICO_DEFAULT_BINARY_INFO or a manually linked custom_pico_binary_info target, type=string, group=pico_binary_info
# PICO_BUILD_DEFINE: PICO_PROGRAM_DESCRIPTION, Provided by PICO_DEFAULT_BINARY_INFO or a manually linked custom_pico_binary_info target, type=string, group=pico_binary_info
# PICO_BUILD_DEFINE: PICO_PROGRAM_URL, Provided by PICO_DEFAULT_BINARY_INFO or a manually linked custom_pico_binary_info target, type=string, group=pico_binary_info
# PICO_BUILD_DEFINE: PICO_PROGRAM_VERSION_STRING, Provided by PICO_DEFAULT_BINARY_INFO or a manually linked custom_pico_binary_info target, type=string, group=pico_binary_info
# PICO_BUILD_DEFINE: PICO_TARGET_NAME, The name of the build target being compiled, type=string, default=target name, group=build
def custom_pico_binary_info(name = None, program_name = None, program_description = None, program_url = None, program_version_string = None, build_target_name = None):
    _all_defines = []
    if program_name != None:
        _all_defines.append('PICO_PROGRAM_NAME=\\"{}\\"'.format(program_name))
    if program_description != None:
        _all_defines.append('PICO_PROGRAM_DESCRIPTION=\\"{}\\"'.format(program_description))
    if program_url != None:
        _all_defines.append('PICO_PROGRAM_URL=\\"{}\\"'.format(program_url))
    if program_version_string != None:
        _all_defines.append('PICO_PROGRAM_VERSION_STRING=\\"{}\\"'.format(program_version_string))

    # TODO: There's no practical way to support this correctly without a
    # `pico_cc_binary` wrapper. Either way, this would be the right place to put
    # it.
    _build_target_name_defines = []
    if build_target_name != None:
        _build_target_name_defines.append('PICO_TARGET_NAME=\\"{}\\"'.format(build_target_name))
    cc_library(
        name = name,
        defines = _all_defines + select({
            "@pico-sdk//bazel/constraint:pico_no_target_name_enabled": [],
            "//conditions:default": _build_target_name_defines,
        }),
        srcs = ["@pico-sdk//src/rp2_common/pico_standard_binary_info:binary_info_srcs"],
        deps = [
            "@pico-sdk//src/rp2_common/pico_standard_binary_info:PICO_BAZEL_BUILD_TYPE",
            "@pico-sdk//src/common/pico_base_headers:version",
            "@pico-sdk//src/common/pico_binary_info",
            "@pico-sdk//src/rp2_common:boot_stage2_config",
        ],
        alwayslink = True,
    )
