# DUT for the bond-survives-disconnect regression test.
#
# Built from BTstack spp_and_gatt_counter example, with the settings the test needs
#   - USB stdio, so picotool can reboot the board between runs
#   - UART stdio, for a serial console on a rig that has one
#   - WANT_HCI_DUMP, so failures come with a packet log
#
# Drive it with test.sh (all modes) or bond_reconnect_test.py in this directory.

if (NOT PICO_CYW43_SUPPORTED)
    return()
endif()
if (NOT TARGET pico_btstack_classic)
    return()
endif()

# BTstack's spp_and_gatt_counter example is the Bluetooth application: it gives
# SPP over BR/EDR *and* an advertising GATT server over LE, so one DUT covers
# all three modes of bond_reconnect_test.py (ctkd / classic / le).
add_executable(pico_btstack_bond_test
    main.c
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.c
)

pico_btstack_make_gatt_header(pico_btstack_bond_test PRIVATE
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.gatt)

target_link_libraries(pico_btstack_bond_test PRIVATE
    pico_stdlib
    pico_cyw43_arch_none  # no lwip needed
    pico_btstack_cyw43
    pico_btstack_ble      # both transports, so CTKD is compiled in
    pico_btstack_classic
)

target_include_directories(pico_btstack_bond_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}  # for btstack_config.h
)

target_compile_definitions(pico_btstack_bond_test PRIVATE
    WANT_HCI_DUMP=1
)

# USB stdio lets picotool reboot the board without touching BOOTSEL
pico_enable_stdio_usb(pico_btstack_bond_test 1)
pico_enable_stdio_uart(pico_btstack_bond_test 1)

pico_add_extra_outputs(pico_btstack_bond_test)
