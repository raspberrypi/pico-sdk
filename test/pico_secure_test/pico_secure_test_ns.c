/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/test.h"
#include "pico/rand.h"
#include "pico/secure.h"
#include "pico/stdio_usb.h"

#include "secure_call_user_callbacks.h"

#include "hardware/pio.h"
#include "addition.pio.h"

uint32_t do_addition(PIO pio, uint sm, uint32_t a, uint32_t b) {
    pio_sm_put_blocking(pio, sm, a);
    pio_sm_put_blocking(pio, sm, b);
    return pio_sm_get_blocking(pio, sm);
}

int ns_repeats = 0;
bool repeating_timer_callback(__unused struct repeating_timer *t) {
    printf("NS Repeat at %lld\n", time_us_64());
    ns_repeats++;
    return true;
}

PICOTEST_MODULE_NAME("NONSECURE", "nonsecure test");

int main() {
    // Request user IRQ from secure, which stdio_usb will use
    user_irq_request_unused_from_secure(1);

    // Start stdio_usb
    stdio_usb_init();

    // Repeating timer
    struct repeating_timer timer;
    add_repeating_timer_ms(100, repeating_timer_callback, NULL, &timer);

    PICOTEST_START();

    PICOTEST_START_SECTION("secure_calls");
    for (int i=0; i < 10; i++) {
        printf("Hello, world, from non-secure!\n");
        PICOTEST_CHECK(rom_secure_call(1, 2, 3, i, SECURE_CALL_PRINT_VALUES) == BOOTROM_OK, "secure call print values failed");
        PICOTEST_CHECK(rom_secure_call(picotest_error_code, 0, 0, 0, SECURE_CALL_UPDATE_RETURN) == BOOTROM_OK, "secure call update return failed");
        sleep_ms(100);
    }
    PICOTEST_CHECK(ns_repeats >= 9, "nonsecure timer not working");
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("pio");
    // Check no PIOs are available
    int sm_check = pio_claim_unused_sm(pio0, false);
    sm_check += pio_claim_unused_sm(pio1, false);
    sm_check += pio_claim_unused_sm(pio2, false);
    printf("PIO sum: %d\n", sm_check);
    PICOTEST_CHECK(sm_check == -3, "PIOs are already available");
    // Request PIO from secure
    int got_pio = pio_request_unused_pio_from_secure();
    printf("Got PIO: %d\n", got_pio);
    PICOTEST_CHECK(got_pio >= 0, "did not get pio");

    PIO pio;
    uint sm;
    uint offset;
    pio_claim_free_sm_and_add_program(&addition_program, &pio, &sm, &offset);
    addition_program_init(pio, sm, offset);

    printf("Doing some random additions:\n");
    for (int i = 0; i < 10; ++i) {
        uint a = get_rand_32() % 100;
        uint b = get_rand_32() % 100;
        uint res = do_addition(pio, sm, a, b);
        printf("%u + %u = %u\n", a, b, res);
        PICOTEST_CHECK(res == a + b, "addition not correct");
    }
    PICOTEST_END_SECTION();

    PICOTEST_START_SECTION("secure_hardfault");
    PICOTEST_CHECK(rom_secure_call(0, 0, 0, 0, SECURE_CALL_COMPLETED) == BOOTROM_OK, "secure call completed failed");
    // Demonstrate triggering a secure fault, by reading secure memory at start of SRAM
    printf("Triggering secure fault by reading secure memory\n");
    volatile uint32_t thing = *(uint32_t*)SRAM_BASE;
    printf("Some secure memory is %08x\n", thing);
    PICOTEST_CHECK(false, "should have triggered hardfault");
    PICOTEST_END_SECTION();


    PICOTEST_END_TEST();
}
