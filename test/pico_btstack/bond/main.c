/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DUT for the bond/reconnect regression test. Runs bond_dut.c, which bonds over
 * BR/EDR via SSP and advertises a GATT server over LE, with both Classic and LE
 * enabled so Cross-Transport Key Derivation is active.
 *
 * Drive it with test.sh, or bond_reconnect_test.py for a single mode.
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

// implemented by bond_dut.c (vendored from btstack example/spp_and_gatt_counter.c)
int btstack_main(int argc, const char *argv[]);

int main(void) {
    stdio_init_all();

    if (cyw43_arch_init() != PICO_OK) {
        panic("failed to initialise cyw43");
    }

    btstack_main(0, NULL);

    // The local name embeds the BD_ADDR once BTstack is up; the address is also
    // printed by BTSTACK_EVENT_STATE handling. Pass it to the test with --addr.
    btstack_run_loop_execute();

    cyw43_arch_deinit();
    return 0;
}
