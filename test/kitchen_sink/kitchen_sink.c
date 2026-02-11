/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#ifndef KITCHEN_SINK_INCLUDE_HEADER
// provided for backwards compatibility for non CMake build systems - just includes enough to compile
#include "hardware/dma.h"
#include "pico/sync.h"
#include "pico/stdlib.h"
#include "pico/util/bitset.h"
#if LIB_PICO_BINARY_INFO
#include "pico/binary_info.h"
#endif
#if LIB_PICO_AON_TIMER
#include "pico/aon_timer.h"
#endif
#else
#include KITCHEN_SINK_INCLUDE_HEADER
#endif

#if LIB_PICO_MBEDTLS
#include "mbedtls/ssl.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#endif

#if LIB_PICO_BINARY_INFO
bi_decl(bi_block_device(
                           BINARY_INFO_MAKE_TAG('K', 'S'),
                           "foo",
                           0x80000,
                           0x40000,
                           NULL,
                           BINARY_INFO_BLOCK_DEV_FLAG_READ | BINARY_INFO_BLOCK_DEV_FLAG_WRITE |
                                   BINARY_INFO_BLOCK_DEV_FLAG_PT_UNKNOWN));
#endif

uint32_t *foo = (uint32_t *) 200;

uint32_t dma_to = 0;
uint32_t dma_from = 0xaaaa5555;

void __noinline spiggle(void) {
    dma_channel_config c = dma_channel_get_default_config(1);
    channel_config_set_bswap(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_ring(&c, true, 13);
    dma_channel_set_config(1, &c, false);
    dma_channel_transfer_from_buffer_now(1, foo, 23);
}

__force_inline int something_inlined(int x) {
    return x * 2;
}

auto_init_mutex(mutex);
auto_init_recursive_mutex(recursive_mutex);

float __attribute__((noinline)) foox(float x, float b) {
    return x * b;
}

int main(void) {
    spiggle();

    stdio_init_all();

    printf("HI %d\n", something_inlined((int)time_us_32()));
    puts("Hello Everything!");
    puts("Hello Everything2!");

    printf("main at %p\n", (void *)main);
    static uint x[2];
    printf("x[0] = %p, x[1] = %p\n", x, x+1);
#ifdef __riscv
    printf("RISC-V\n");
#else
    printf("ARM\n");
#endif
#ifdef KITCHEN_SINK_ID
    puts(KITCHEN_SINK_ID);
#endif
    hard_assert(mutex_try_enter(&mutex, NULL));
    hard_assert(!mutex_try_enter(&mutex, NULL));
    hard_assert(recursive_mutex_try_enter(&recursive_mutex, NULL));
    hard_assert(recursive_mutex_try_enter(&recursive_mutex, NULL));
    typedef bitset_type_t(47) foop_t;
//#define WOOP encoded_bitset_of3(1, 27, 32)
#define WOOP encoded_bitset_of5(1, 27, 32, 40, 3)
    encoded_bitset_foreach(WOOP, printf("Flarn %d\n", bit));
    foop_t fooper;
    bitset_init(&fooper, foop_t, 47, 0);
    encoded_bitset_foreach(WOOP, bitset_set_bit(&fooper.bitset, bit));
    for(uint i=0;i<bitset_size(&fooper.bitset);i++) {
        if (bitset_get_bit(&fooper.bitset, i)) printf("Have at %d\n", i);
    }
    printf("%f\n", foox(1.3f, 2.6f));
#if LIB_PICO_AON_TIMER
    aon_timer_start_with_timeofday();
    void spoop(void);
    struct timespec ts;
    ts.tv_sec = 6;
    ts.tv_nsec = 1000000000 / 2;
    aon_timer_enable_alarm(&ts, spoop, false);
    while (true) {
        aon_timer_get_time(&ts);
        printf("%ld %ld\n", (long)ts.tv_sec, ts.tv_nsec);
        busy_wait_ms(4000);
    }
#endif

#ifdef EXTRA_DATA_SECTION
    static int extra_data __attribute__((section(".extra_data"))) = 12345678;
    printf("extra_data before load = %d\n", extra_data);

    extern uint32_t __extra_data_source__;
    extern uint32_t __extra_data_start__;
    extern uint32_t __extra_data_end__;
    uint32_t stored_words = (uint32_t)(&__extra_data_end__ - &__extra_data_start__);
    memcpy(&__extra_data_start__, &__extra_data_source__, 4 * stored_words);

    printf("extra_data after load = %d\n", extra_data);
#endif
#ifndef __riscv
    // this should compile as we are Cortex M0+
    pico_default_asm ("SVC #3");
#endif
}

#if LIB_PICO_AON_TIMER
void spoop(void) {
    printf("XXXX YARGLE XXXX\n");
}
#endif
