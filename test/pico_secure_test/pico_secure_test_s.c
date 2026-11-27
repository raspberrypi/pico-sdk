/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "pico/secure.h"
#include "pico/test.h"

#include "secure_call_user_callbacks.h"


bool repeating_timer_callback(__unused struct repeating_timer *t) {
    watchdog_update();
    printf("Secure repeat at %lld\n", time_us_64());
    return true;
}


PICOTEST_MODULE_NAME("SECURE", "secure test");


static bool nonsecure_completed = false;

static inline int end_test_func(void) {
    if (!nonsecure_completed) picotest_error_code = -1;
    PICOTEST_END_TEST();
}

// test ends with deliberate hardfault
void hardfault_callback(void) {
    exit(end_test_func());
}

int secure_call_user_callback(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t fn) {
    switch (fn) {
        case SECURE_CALL_PRINT_VALUES:
            printf("NS called secure_call with %d %d %d %d\n", a, b, c, d);
            return BOOTROM_OK;
        case SECURE_CALL_UPDATE_RETURN:
            if (picotest_error_code == 0) picotest_error_code = a;
            return BOOTROM_OK;
        case SECURE_CALL_COMPLETED:
            nonsecure_completed = true;
            return BOOTROM_OK;
        default:
            return BOOTROM_ERROR_INVALID_ARG;
    }
}

int main() {
    stdio_init_all();

    // If this was a watchdog reboot, reset to USB boot
    if (watchdog_enable_caused_reboot()) {
        printf("This was a watchdog reboot - exiting\n");
        return -1;
    }

    // Create a repeating timer to update the watchdog every 1000ms
    struct repeating_timer timer;
    watchdog_enable(1100, true);
    add_repeating_timer_ms(-1000, repeating_timer_callback, NULL, &timer);
    
    // Create user callback
    rom_secure_call_add_user_callback(secure_call_user_callback, SECURE_CALL_CALLBACKS_MASK);

#if !PICO_SECURITY_SPLIT_NO_FLASH
    // Get boot partition
    boot_info_t info;
    rom_get_boot_info(&info);
    printf("Boot partition: %d\n", info.partition);

    // Roll QMI to matching Non-Secure partition, as Non-Secure runs from XIP
    int ns_partition = rom_get_owned_partition(info.partition);
    printf("Matching Non-Secure partition: %d\n", ns_partition);
    int rc = flash_roll_qmi_to_partition(ns_partition);
    printf("Rolled QMI to Non-Secure partition, rc=%d\n", rc);
#endif

    // Configure SAU regions
    secure_sau_configure_split();

    // Enable SAU
    secure_sau_set_enabled(true);
    printf("SAU Configured & Enabled\n");

    // Install default hardfault handler, with callback to reset to USB boot
    secure_install_default_hardfault_handler(hardfault_callback);

    // Launch Non-Secure binary
    secure_launch_nonsecure_binary_default();

    // Should never return from non-secure code
    printf("ERROR: Shouldn't return from non-secure code\n");
}
