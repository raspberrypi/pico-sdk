# DUT for the bond-survives-disconnect regression test.
#
# Built from bond_dut.c here, with the settings the test needs, so nothing in
# pico-examples has to be modified:
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

add_executable(pico_btstack_bond_test
    main.c
    bond_dut.c
)

# bond_dut.c is a vendored copy of btstack's spp_and_gatt_counter example. It
# provides SPP over BR/EDR *and* an advertising GATT server over LE, so one DUT
# covers all three modes of bond_reconnect_test.py (ctkd / classic / le).
# The only change from upstream is sm_set_authentication_requirements(), which
# must request bonding for LE keys to persist.
pico_btstack_make_gatt_header(pico_btstack_bond_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/bond_dut.gatt)

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
