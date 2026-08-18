/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DUT for the bond/reconnect regression test.
 *
 * The Bluetooth application is BTstack's spp_and_gatt_counter example
 * It gives us SPP over BR/EDR and an advertising GATT server over LE,
 * with both Classic and LE enabled so Cross-Transport Key Derivation
 * is active.
 *
 * Drive it with test.sh, or bond_reconnect_test.py for a single mode.
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

// implemented by lib/btstack/example/spp_and_gatt_counter.c
int btstack_main(int argc, const char *argv[]);

// Name advertised over LE and used as the classic local name, so the test can
// find the board with --name rather than a hard-coded address.
#define BOND_DUT_NAME "PicoBondTest"

// Replacement advertising data carrying BOND_DUT_NAME
static const uint8_t bond_adv_data[] = {
    // Flags: general discoverable, dual mode
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x02,
    // Name
    0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'P', 'i', 'c', 'o', 'B', 'o', 'n', 'd', 'T', 'e', 's', 't',
};

int main(void) {
    stdio_init_all();

    if (cyw43_arch_init() != PICO_OK) {
        panic("failed to initialise cyw43");
    }

    btstack_main(0, NULL);

    // The example calls sm_set_authentication_requirements(0)
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTION);

    // Give the board a name the test can search for, Classic and LE.
    gap_set_local_name(BOND_DUT_NAME " 00:00:00:00:00:00");
    gap_advertisements_set_data(sizeof(bond_adv_data), (uint8_t *)bond_adv_data);

    btstack_run_loop_execute();

    cyw43_arch_deinit();
    return 0;
}
