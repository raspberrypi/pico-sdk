load("@pico-sdk//bazel:defs.bzl", "incompatible_with_config")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pico_mbedtls_library",
    srcs = glob(
        ["library/*.c"],
        exclude = ["*mbedtls.c"],
    ),
    hdrs = glob(
        include = [
            "include/**/*.h",
            "library/*.h",
        ],
    ),
    includes = ["include"],
    target_compatible_with = incompatible_with_config("@pico-sdk//bazel/constraint:pico_mbedtls_config_unset"),
    deps = [
        "@pico-sdk//bazel/config:PICO_MBEDTLS_CONFIG",
        "@pico-sdk//src/rp2_common/pico_mbedtls:pico_mbedtls_config",
    ],
)
