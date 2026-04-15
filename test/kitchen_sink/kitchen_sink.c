/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef KITCHEN_SINK_INCLUDE_HEADER
// provided for backwards compatibility for non CMake build systems - just includes enough to compile
#include "hardware/dma.h"
#include "hardware/exception.h"
#include "pico/sync.h"
#include "pico/stdlib.h"
#if LIB_PICO_BINARY_INFO
#include "pico/binary_info.h"
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

void svc_call(void) {
    puts("PASSED");
    exit(0);
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
    printf("%f\n", foox(1.3f, 2.6f));
#ifdef EXTRA_DATA_SECTION
    extern uint32_t __extra_end_variable__;
    printf("__extra_end_variable__ = %p\n", (void *)&__extra_end_variable__);
#if EXTRA_DATA_SECTION > 1
    extern uint32_t __overlays_start__;
    uint32_t stored_words;

    static int overlay_first __attribute__((section(".overlay_first"))) = 12345678;
    printf("overlay_first before load = %d\n", overlay_first);
    static int overlay_second_one __attribute__((section(".overlay_second"))) = 34567890;
    static int overlay_second_two __attribute__((section(".overlay_second"))) = 56789012;
    printf("overlay_second before load = %d, %d\n", overlay_second_one, overlay_second_two);

    extern uint32_t __load_start_overlay_second;
    extern uint32_t __load_stop_overlay_second;
    stored_words = (uint32_t)(&__load_stop_overlay_second - &__load_start_overlay_second);
    memcpy(&__overlays_start__, &__load_start_overlay_second, 4 * stored_words);
    printf("overlay_second after load = %d, %d\n", overlay_second_one, overlay_second_two);

    extern uint32_t __load_start_overlay_first;
    extern uint32_t __load_stop_overlay_first;
    stored_words = (uint32_t)(&__load_stop_overlay_first - &__load_start_overlay_first);
    memcpy(&__overlays_start__, &__load_start_overlay_first, 4 * stored_words);
    printf("overlay_first after load = %d\n", overlay_first);
    printf("overlay_second after overlay_first load = %d, %d\n", overlay_second_one, overlay_second_two);
#else
    static int extra_data __attribute__((section(".extra_data"))) = 12345678;
    printf("extra_data before load = %d\n", extra_data);

    extern uint32_t __extra_data_source__;
    extern uint32_t __extra_data_start__;
    extern uint32_t __extra_data_end__;
    uint32_t stored_words = (uint32_t)(&__extra_data_end__ - &__extra_data_start__);
    memcpy(&__extra_data_start__, &__extra_data_source__, 4 * stored_words);

    printf("extra_data after load = %d\n", extra_data);
#endif
#endif
#ifndef __riscv
    exception_set_exclusive_handler(SVCALL_EXCEPTION, svc_call);
    // this should compile as we are Cortex-M
    pico_default_asm ("SVC #3");
#else
    puts("PASSED");
#endif
}
