load("@pico-sdk//bazel:defs.bzl", "incompatible_with_config")

package(default_visibility = ["//visibility:public"])

_DISABLE_WARNINGS = [
    "-Wno-cast-qual",
    "-Wno-format",
    "-Wno-maybe-uninitialized",
    "-Wno-null-dereference",
    "-Wno-sign-compare",
    "-Wno-stringop-overflow",
    "-Wno-suggest-attribute=format",
    "-Wno-type-limits",
    "-Wno-unused-parameter",
]

cc_library(
    name = "pico_btstack_base",
    srcs = [
        "3rd-party/md5/md5.c",
        "3rd-party/micro-ecc/uECC.c",
        "3rd-party/rijndael/rijndael.c",
        "3rd-party/segger-rtt/SEGGER_RTT.c",
        "3rd-party/segger-rtt/SEGGER_RTT_printf.c",
        "3rd-party/yxml/yxml.c",
        "platform/embedded/btstack_tlv_flash_bank.c",
        "platform/embedded/hci_dump_embedded_stdout.c",
        "platform/embedded/hci_dump_segger_rtt_stdout.c",
        "src/ad_parser.c",
        "src/btstack_audio.c",
        "src/btstack_base64_decoder.c",
        "src/btstack_crypto.c",
        "src/btstack_hid_parser.c",
        "src/btstack_linked_list.c",
        "src/btstack_memory.c",
        "src/btstack_memory_pool.c",
        "src/btstack_resample.c",
        "src/btstack_ring_buffer.c",
        "src/btstack_run_loop.c",
        "src/btstack_run_loop_base.c",
        "src/btstack_slip.c",
        "src/btstack_tlv.c",
        "src/btstack_tlv_none.c",
        "src/btstack_util.c",
        "src/hci.c",
        "src/hci_cmd.c",
        "src/hci_dump.c",
        "src/hci_event.c",
        "src/l2cap.c",
        "src/l2cap_signaling.c",
        "src/mesh/gatt-service/mesh_provisioning_service_server.c",
        "src/mesh/gatt-service/mesh_proxy_service_server.c",
    ],
    hdrs = glob(["**/*.h"]),
    copts = _DISABLE_WARNINGS,
    includes = [
        ".",
        "3rd-party/md5",
        "3rd-party/micro-ecc",
        "3rd-party/rijndael",
        "3rd-party/segger-rtt",
        "3rd-party/yxml",
        "platform/embedded",
        "src",
    ],
    target_compatible_with = incompatible_with_config(
        "@pico-sdk//bazel/constraint:pico_btstack_config_unset",
    ),
    deps = ["@pico-sdk//bazel/config:PICO_BTSTACK_CONFIG"],
)

cc_library(
    name = "pico_btstack_ble",
    srcs = [
        "src/ble/att_db.c",
        "src/ble/att_db_util.c",
        "src/ble/att_dispatch.c",
        "src/ble/att_server.c",
        "src/ble/gatt-service/ancs_client.c",
        "src/ble/gatt-service/battery_service_client.c",
        "src/ble/gatt-service/battery_service_server.c",
        "src/ble/gatt-service/cycling_power_service_server.c",
        "src/ble/gatt-service/cycling_speed_and_cadence_service_server.c",
        "src/ble/gatt-service/device_information_service_client.c",
        "src/ble/gatt-service/device_information_service_server.c",
        "src/ble/gatt-service/heart_rate_service_server.c",
        "src/ble/gatt-service/hids_client.c",
        "src/ble/gatt-service/hids_device.c",
        "src/ble/gatt-service/nordic_spp_service_server.c",
        "src/ble/gatt-service/ublox_spp_service_server.c",
        "src/ble/gatt_client.c",
        "src/ble/le_device_db_memory.c",
        "src/ble/le_device_db_tlv.c",
        "src/ble/sm.c",
    ],
    copts = _DISABLE_WARNINGS,
    deps = [":pico_btstack_base"],
)

cc_library(
    name = "pico_btstack_classic",
    srcs = [
        "src/classic/a2dp.c",
        "src/classic/a2dp_sink.c",
        "src/classic/a2dp_source.c",
        "src/classic/avdtp.c",
        "src/classic/avdtp_acceptor.c",
        "src/classic/avdtp_initiator.c",
        "src/classic/avdtp_sink.c",
        "src/classic/avdtp_source.c",
        "src/classic/avdtp_util.c",
        "src/classic/avrcp.c",
        "src/classic/avrcp_browsing.c",
        "src/classic/avrcp_browsing_controller.c",
        "src/classic/avrcp_browsing_target.c",
        "src/classic/avrcp_controller.c",
        "src/classic/avrcp_cover_art_client.c",
        "src/classic/avrcp_media_item_iterator.c",
        "src/classic/avrcp_target.c",
        "src/classic/btstack_cvsd_plc.c",
        "src/classic/btstack_link_key_db_tlv.c",
        "src/classic/btstack_sbc_plc.c",
        "src/classic/device_id_server.c",
        "src/classic/gatt_sdp.c",
        "src/classic/goep_client.c",
        "src/classic/goep_server.c",
        "src/classic/hfp.c",
        "src/classic/hfp_ag.c",
        "src/classic/hfp_gsm_model.c",
        "src/classic/hfp_hf.c",
        "src/classic/hfp_msbc.c",
        "src/classic/hid_device.c",
        "src/classic/hid_host.c",
        "src/classic/hsp_ag.c",
        "src/classic/hsp_hs.c",
        "src/classic/obex_iterator.c",
        "src/classic/obex_message_builder.c",
        "src/classic/obex_parser.c",
        "src/classic/pan.c",
        "src/classic/pbap_client.c",
        "src/classic/rfcomm.c",
        "src/classic/sdp_client.c",
        "src/classic/sdp_client_rfcomm.c",
        "src/classic/sdp_server.c",
        "src/classic/sdp_util.c",
        "src/classic/spp_server.c",
    ],
    copts = _DISABLE_WARNINGS,
    deps = [":pico_btstack_base"],
)

cc_library(
    name = "pico_btstack_sbc_encoder",
    srcs = [
        "3rd-party/bluedroid/encoder/srce/sbc_analysis.c",
        "3rd-party/bluedroid/encoder/srce/sbc_dct.c",
        "3rd-party/bluedroid/encoder/srce/sbc_dct_coeffs.c",
        "3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_mono.c",
        "3rd-party/bluedroid/encoder/srce/sbc_enc_bit_alloc_ste.c",
        "3rd-party/bluedroid/encoder/srce/sbc_enc_coeffs.c",
        "3rd-party/bluedroid/encoder/srce/sbc_encoder.c",
        "3rd-party/bluedroid/encoder/srce/sbc_packing.c",
        "src/classic/btstack_sbc_encoder_bluedroid.c",
    ],
    copts = _DISABLE_WARNINGS,
    includes = ["3rd-party/bluedroid/decoder/include"],
    deps = [":pico_btstack_base"],
)

cc_library(
    name = "pico_btstack_bnep_lwip",
    srcs = [
        "platform/lwip/bnep_lwip.c",
        "src/classic/bnep.c",
    ],
    copts = _DISABLE_WARNINGS,
    includes = ["platform/lwip"],
    deps = [":pico_btstack_base"],
)

cc_library(
    name = "pico_btstack_bnep_lwip_sys_freertos",
    srcs = [
        "platform/lwip/bnep_lwip.c",
        "src/classic/bnep.c",
    ],
    copts = _DISABLE_WARNINGS,
    defines = [
        "LWIP_PROVIDE_ERRNO=1",
        "PICO_LWIP_CUSTOM_LOCK_TCPIP_CORE=1",
    ],
    includes = [
        "platform/freertos",
        "platform/lwip",
    ],
    deps = [":pico_btstack_base"],
)
