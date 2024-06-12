package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cyw43_driver",
    hdrs = glob(["**/*.h"]),
    includes = [
        "src",
        "firmware",
    ],
    defines = ["CYW43_ENABLE_BLUETOOTH=1"],
    deps = [
        "@pico-sdk//src/rp2_common/pico_cyw43_driver:cyw43_configport",
        "@pico-sdk//src/rp2_common/pico_lwip",
    ],
    target_compatible_with = select({
        "@pico-sdk//bazel/constraint:cyw43_wireless": [],
        "@pico-sdk//bazel/constraint:is_pico_w": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    srcs = [
        "src/cyw43_ll.c",
        "src/cyw43_stats.c",
        "src/cyw43_lwip.c",
        "src/cyw43_ctrl.c",
    ]
)
