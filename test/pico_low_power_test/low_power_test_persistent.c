/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_test_common.h"


static char powman_last_pwrup[100];
static char powman_last_pstate_linker[100];
static char powman_last_pstate_runtime[100];

int __persistent_data(my_number);

// Increase this size to see the cache hit rate decrease, when using XIP_SRAM for persistent data
char __persistent_data(large_thing)[0x1000];

int main() {
    stdio_init_all();
    status_led_init();

    my_number = 12345;
    memset(large_thing, 0x55, sizeof(large_thing));

    pstate_bitset_t pstate = pstate_bitset_none();

    // Runtime check because PICO_LOW_POWER_PERSISTENT_PSTATE_STATIC=0
    low_power_persistent_pstate_get(&pstate);
    pstate_resume_func_common(&pstate, powman_last_pwrup, powman_last_pstate_runtime);
    printf("Persistent data would keep (%s) memory on\n", powman_last_pstate_runtime);

    // Linker check
    extern unsigned char __persistent_data_pstate__[]; // linker script provides this
    pstate_bitset_from_uint32(&pstate, (uint32_t)__persistent_data_pstate__);
    pstate_resume_func_common(&pstate, powman_last_pwrup, powman_last_pstate_linker);
    printf("Persistent data would keep (%s) memory on\n", powman_last_pstate_linker);

    if (strcmp(powman_last_pstate_runtime, powman_last_pstate_linker) != 0) {
        printf("ERROR: Mismatch between linker and runtime values\n");
    } else {
        printf("PASSED\n");
    }

    return 0;
}