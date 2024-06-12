package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pico_lwip_core",
    hdrs = glob(["**/*.h"]),
    includes = ["src/include"],
    srcs = glob(["src/core/*.c"]),
    deps = [
        "@pico-sdk//bazel/config:PICO_LWIP_CONFIG",
        "@pico-sdk//src/rp2_common/pico_lwip:pico_lwip_config",
    ],
    target_compatible_with = select({
        "@pico-sdk//bazel/constraint:pico_lwip_config_unset": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "pico_lwip_core4",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/core/ipv4/*.c"]),
)

cc_library(
    name = "pico_lwip_core6",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/core/ipv6/*.c"]),
)

cc_library(
    name = "pico_lwip_api",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/api/*.c"]),
)

cc_library(
    name = "pico_lwip_netif",
    deps = [":pico_lwip_core"],
    srcs = [
        "src/netif/ethernet.c",
        "src/netif/bridgeif.c",
        "src/netif/bridgeif_fdb.c",
        "src/netif/slipif.c",
    ],
)

cc_library(
    name = "pico_lwip_sixlowpan",
    deps = [":pico_lwip_core"],
    srcs = [
        "src/netif/lowpan6_common.c",
        "src/netif/lowpan6.c",
        "src/netif/lowpan6_ble.c",
        "src/netif/zepif.c",
    ],
)

cc_library(
    name = "pico_lwip_ppp",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/netif/ppp/*/*.c"]),
)

cc_library(
    name = "pico_lwip_snmp",
    deps = [":pico_lwip_core"],
    srcs = glob(
        ["src/apps/snmp/*.c"],
        # mbedtls is provided through pico_lwip_mbedtls.
        exclude = ["*mbedtls.c"],
    ),
)

cc_library(
    name = "pico_lwip_http",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/apps/http/*.c"]),
)

cc_library(
    name = "pico_lwip_makefsdata",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/http/makefsdata/makefsdata.c"],
)

cc_library(
    name = "pico_lwip_iperf",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/lwiperf/lwiperf.c"],
)

cc_library(
    name = "pico_lwip_smtp",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/smtp/smtp.c"],
)

cc_library(
    name = "pico_lwip_sntp",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/sntp/sntp.c"],
)

cc_library(
    name = "pico_lwip_mdns",
    deps = [":pico_lwip_core"],
    srcs = glob(["src/apps/mdns/*.c"]),
)

cc_library(
    name = "pico_lwip_netbios",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/netbiosns/netbiosns.c"],
)

cc_library(
    name = "pico_lwip_tftp",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/tftp/tftp.c"],
)

cc_library(
    name = "pico_lwip_mbedtls",
    deps = [":pico_lwip_core"],
    srcs = [
        "src/apps/altcp_tls/altcp_tls_mbedtls.c",
        "src/apps/altcp_tls/altcp_tls_mbedtls_mem.c",
        "src/apps/snmp/snmpv3_mbedtls.c",
    ],
)

cc_library(
    name = "pico_lwip_mqttt",
    deps = [":pico_lwip_core"],
    srcs = ["src/apps/mqtt/mqtt.c"],
)

cc_library(
    name = "pico_lwip",
    deps = [
        ":pico_lwip_core",
        ":pico_lwip_core4",
        ":pico_lwip_core6",
        ":pico_lwip_api",
        ":pico_lwip_netif",
        ":pico_lwip_sixlowpan",
        ":pico_lwip_ppp",
    ],
)

cc_library(
    name = "pico_lwip_contrib_freertos",
    includes = ["ports/freertos/include"],
    srcs = ["ports/freertos/sys_arch.c"],
    deps = [":pico_lwip_core"],
)
