/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

volatile bool failed;
volatile uint32_t count[3];
volatile bool done;

bool timer_callback(repeating_timer_t *t) {
    count[0]++;
    static int z;
    for (int i=0; i<100;i++) {
        z += 23;
        int a = z / 7;
        int b = z % 7;
        if (z != a * 7 + b) {
            failed = true;
        }
    }
    return !done;
}

void do_dma_start(uint ch) {
    static uint32_t word[2];
    assert(ch < 2);
    dma_channel_config c = dma_channel_get_default_config(ch);
    // todo remove this; landing in a separate PR
#ifndef DREQ_DMA_TIMER0
#define DREQ_DMA_TIMER0 0x3b
#endif
    channel_config_set_dreq(&c, DREQ_DMA_TIMER0);
    dma_channel_configure(ch, &c, &word[ch], &word[ch], 513 + ch * 23, true);
}

void test_irq_handler0() {
    count[1]++;
    dma_hw->ints0 |= 1u;
    static uint z;
    for (int i=0; i<80;i++) {
        z += 31;
        uint a = z / 11;
        uint b = z % 11;
        if (z != a * 11 + b) {
            failed = true;
        }
    }
    if (done) dma_channel_abort(0);
    else      do_dma_start(0);
}

void test_irq_handler1() {
    static uint z;
    dma_hw->ints1 |= 2u;
    count[2]++;
    for (int i=0; i<130;i++) {
        z += 47;
        uint a = z / -13;
        uint b = z % -13;
        if (z != a * -13 + b) {
            failed = true;
        }
        static uint64_t z64;
        z64 -= 47;
        uint64_t a64 = z64 / -13;
        uint64_t b64 = z64 % -13;
        if (z64 != a64 * -13 + b64) {
            failed = true;
        }
    }
    if (done) dma_channel_abort(1);
    else      do_dma_start(1);
}

void test_nesting() {
    uint z = 0;

    // We have 3 different IRQ handlers, one for timer, two for DMA completion (on DMA_IRQ0/1)
    // thus we expect re-entrancy even between IRQs
    //
    // They all busily make use of the dividers, to expose any issues with nested use

    repeating_timer_t timer;
    add_repeating_timer_us(529, timer_callback, NULL, &timer);
    irq_set_exclusive_handler(DMA_IRQ_0, test_irq_handler0);
    irq_set_exclusive_handler(DMA_IRQ_1, test_irq_handler1);

    dma_set_irq0_channel_mask_enabled(1u, true);
    dma_set_irq1_channel_mask_enabled(2u, true);
    dma_hw->timer[0] = (1 << 16) | 32; // run at 1/32 system clock

    irq_set_enabled(DMA_IRQ_0, 1);
    irq_set_enabled(DMA_IRQ_1, 1);
    do_dma_start(0);
    do_dma_start(1);
    absolute_time_t end = delayed_by_ms(get_absolute_time(), 2000);
    int count_local=0;
    while (!time_reached(end)) {
        for(uint i=0;i<100;i++) {
            z += 31;
            uint a = z / 11;
            uint b = z % 11;
            if (z != a * 11 + b) {
                failed = true;
            }
        }
        count_local++;
    }
    done = true;
    cancel_repeating_timer(&timer);
    printf("%d: %d %d %d\n", count_local, (int)count[0], (int)count[1], (int)count[2]);

    // make sure all the IRQs ran
    if (!(count_local && count[0] && count[1] && count[2])) {
        printf("DID NOT RUN\n");
        exit(1);
    }
    if (failed) {
        printf("FAILED\n");
        exit(1);
    }
}

int main() {
#ifndef uart_default
#warning test/pico_divider requires a default uart
#else
    stdio_init_all();
#endif
    test_nesting();
    printf("PASSED\n");
    return 0;
}

