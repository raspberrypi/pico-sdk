# DUT for https://github.com/raspberrypi/pico-sdk/issues/2725
#
# Identical to the bond test DUT, but additionally links pico_multicore and
# never launches core 1. This broke BTstack TLV persistence as flash_safe_execute()
# could not make core 1 safe because core 1 never initialised a lockout victim.
# So every write was skipped and link keys were silently not stored.
#
# Fixed in SDK 2.3.0 by #3019, which added PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED
# (default 1) and switched flash.c to multicore_lockout_ready().
#
# Set PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED=0 to restore the old
# behaviour and reproduce the bug on a current SDK:
#
#   cmake -B build -DPICO_BOARD=pico2_w -DPICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED_OVERRIDE=0
#
# Drive it with ../bond/test.sh, which finds the DUT by advertised name.

if (NOT PICO_CYW43_SUPPORTED)
    return()
endif()
if (NOT TARGET pico_btstack_classic)
    return()
endif()

set(BOND_DIR ${CMAKE_CURRENT_LIST_DIR}/../bond)

# Sources and BTstack config are shared with the bond test, this just links to pico_multicore
add_executable(pico_btstack_bond_multicore_test
    ${BOND_DIR}/main.c
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.c
)

pico_btstack_make_gatt_header(pico_btstack_bond_multicore_test PRIVATE
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.gatt)

target_link_libraries(pico_btstack_bond_multicore_test PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_ble
    pico_btstack_classic
    pico_multicore   # linked but core 1 is never launched
)

target_include_directories(pico_btstack_bond_multicore_test PRIVATE
    ${BOND_DIR}  # for btstack_config.h
)

target_compile_definitions(pico_btstack_bond_multicore_test PRIVATE
    WANT_HCI_DUMP=1
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=10000
)

# See https://github.com/raspberrypi/pico-sdk/issues/2725
if (DEFINED PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED_OVERRIDE)
    target_compile_definitions(pico_btstack_bond_multicore_test PRIVATE
        PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED=${PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED_OVERRIDE}
    )
endif()

pico_enable_stdio_usb(pico_btstack_bond_multicore_test 1)
pico_enable_stdio_uart(pico_btstack_bond_multicore_test 1)

pico_add_extra_outputs(pico_btstack_bond_multicore_test)
