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
#include "pico/util/fixed_bitset.h"
#if LIB_PICO_BINARY_INFO
#include "pico/binary_info.h"
#endif
#if LIB_PICO_AON_TIMER
#include "pico/aon_timer.h"
#endif
#if !PICO_RP2040
#include "hardware/flash.h"
#include "hardware/psram.h"
#include "hardware/xip_cache.h"
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

#ifdef FIXED_PSRAM_SIZE
int __in_psram("foo") foo_psram = 23;
char __uninitialized_psram("bar") bar_psram[0x8000];
#if defined(TINY_PSRAM) || defined(SMALL_PSRAM)
void make_tiny_psram(void) {
#if defined(TINY_PSRAM)
    // Override flash_devinfo cs size to be tiny, so bar_psram doesn't fit in it
    flash_devinfo_set_cs_size(1, FLASH_DEVINFO_SIZE_8K);
#elif defined(SMALL_PSRAM)
    // Override flash_devinfo cs size to be small, so bar_psram fits but int_buffer doesn't
    flash_devinfo_set_cs_size(1, FLASH_DEVINFO_SIZE_128K);
#endif
    // Still auto-detect CS pin, as we don't know that
    #if PICO_RP2350
    #if PICO_RP2350A
    uint8_t cs_gpios[] = {0, 8, 19};
    #else
    uint8_t cs_gpios[] = {0, 8, 19, 47};
    #endif
    #else
    // Unknown platform, just try 0
    uint8_t cs_gpios[] = {0}
    #endif
    psram_detect_cs_and_size(cs_gpios, count_of(cs_gpios));
}
PICO_RUNTIME_INIT_FUNC_RUNTIME(make_tiny_psram, "11000");
#endif
#endif

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

uint __noinline const_funcs_returning_int(__unused float x, int n) {
    return __fast_mul(n, 7)
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
           + float2fix(x, 7)
           + float2ufix(x, 7)
#endif
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
           + float2fix_z(x, 7)
           + float2ufix_z(x, 7)
#endif
        ;
}

uint __noinline non_const_funcs_returning_int(__unused float x, int n) {
    return __fast_mul(n, n)
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_M_CONVERSIONS
           + float2fix(x, n)
           + float2ufix(x, n)
#endif
#if PICO_FLOAT_HAS_FLOAT_TO_FIX32_Z_CONVERSIONS
           + float2fix_z(x, n)
           + float2ufix_z(x, n)
#endif
        ;
}

#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
float __noinline const_funcs_returning_float(int n) {
    return fix2float(n, 7) + ufix2float(n, 7);
}

float __noinline non_const_funcs_returning_float(int n) {
    return fix2float(n, n) + ufix2float(n, n);
}
#endif

void svc_call(void) {
    puts("PASSED");
    exit(0);
}

#if LIB_PICO_AON_TIMER
static bool aon_timer_done = false;
void spoop(void) {
    printf("XXXX YARGLE XXXX\n");
    aon_timer_done = true;
}
#endif

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
    printf("%u %u\n", const_funcs_returning_int(3.7f, 7), non_const_funcs_returning_int(3.7f, 7));
#if PICO_FLOAT_HAS_FIX32_TO_FLOAT_CONVERSIONS
    printf("%f %f\n", const_funcs_returning_float(7), non_const_funcs_returning_float(7));
#endif
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
#if LIB_PICO_AON_TIMER
    aon_timer_start_with_timeofday();
    struct timespec ts;
    ts.tv_sec = 2;
    ts.tv_nsec = 1000000000 / 2;
    aon_timer_enable_alarm(&ts, spoop, false);
    while (!aon_timer_done) {
        aon_timer_get_time(&ts);
        printf("%ld %ld\n", (long)ts.tv_sec, ts.tv_nsec);
        busy_wait_ms(500);
    }
#endif

#if PICO_PSRAM_SIZE_BYTES
    psram_or_malloc("z0000", char, char_buffer, 0x8000);
    memset(char_buffer, 0x55, 0x8000);
    printf("char_buffer in %s at %p\n", char_buffer < (char*)SRAM_BASE ? "PSRAM" : "Normal SRAM", char_buffer);
    psram_or_free(char_buffer);
    psram_or_malloc("z0001", int, int_buffer, 0x8000);
    memset(int_buffer, 0x55, 0x8000 * sizeof(int));
    printf("int_buffer in %s at %p\n", int_buffer < (int*)SRAM_BASE ? "PSRAM" : "Normal SRAM", int_buffer);
    psram_or_free(int_buffer);
#endif

#ifdef FIXED_PSRAM_SIZE
    if (psram_is_available()) {
        printf("PSRAM is available\n");
        memset(bar_psram, 0x55, sizeof(bar_psram));
        printf("foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        if (foo_psram != 23 || bar_psram[0] != 0x55) {
            printf("ERROR: foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        }
        // Make sure the write actually went to PSRAM
        xip_cache_clean_all();
        xip_cache_invalidate_all();
        if (foo_psram != 23 || bar_psram[0] != 0x55) {
            printf("ERROR: after flush foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        }
        // Check PSRAM still works after flash functions
        flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
        if (foo_psram != 23 || bar_psram[0] != 0x55) {
            printf("ERROR: after erase foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        }
        foo_psram = 27;
        memset(bar_psram, 0xab, sizeof(bar_psram));
        // Make sure the write actually went to PSRAM
        xip_cache_clean_all();
        xip_cache_invalidate_all();
        printf("foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        if (foo_psram != 27 || bar_psram[0] != 0xab) {
            printf("ERROR: after program foo_psram = %d, bar_psram = %02x\n", foo_psram, bar_psram[0]);
        }
    } else {
        printf("PSRAM not available\n");
    }
#elif !PICO_RP2040
    if (psram_is_available()) {
        printf("PSRAM is available, size = 0x%x\n", psram_get_size());
        size_t psram_size = psram_get_size();
        // Fill each half with different data, to check wrapping isn't ocurring
        char *foo_psram = (char*)(XIP_BASE + 0x01000000);
        size_t foo_psram_size = psram_size / 2;
        char *bar_psram = foo_psram + foo_psram_size;
        size_t bar_psram_size = foo_psram_size;
        memset(foo_psram, 0x55, foo_psram_size);
        memset(bar_psram, 0xab, bar_psram_size);
        printf("foo_psram = %02x, bar_psram = %02x\n", foo_psram[0], bar_psram[0]);
        if (foo_psram[0] != 0x55 || bar_psram[0] != 0xab) {
            printf("ERROR: foo_psram = %02x, bar_psram = %02x\n", foo_psram[0], bar_psram[0]);
        }
        // Make sure the write actually went to PSRAM
        xip_cache_clean_all();
        xip_cache_invalidate_all();
        if (foo_psram[0] != 0x55 || bar_psram[0] != 0xab) {
            printf("ERROR: after flush foo_psram = %02x, bar_psram = %02x\n", foo_psram[0], bar_psram[0]);
        }
        // Check PSRAM still works after flash functions
        flash_range_erase(PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
        if (foo_psram[0] != 0x55 || bar_psram[0] != 0xab) {
            printf("ERROR: after erase foo_psram = %02x, bar_psram = %02x\n", foo_psram[0], bar_psram[0]);
        }
        memset(foo_psram, 0xac, foo_psram_size);
        memset(bar_psram, 0x56, bar_psram_size);
        // Make sure the write actually went to PSRAM
        xip_cache_clean_all();
        xip_cache_invalidate_all();
        printf("foo_psram = %02x, bar_psram = %02x\n", foo_psram[0], bar_psram[0]);
        if (foo_psram[0] != 0xac || bar_psram[0] != 0x56) {
            printf("ERROR: after program bar_psram = %02x\n", bar_psram[0]);
        }
    } else {
    #if PICO_AUTO_DETECT_PSRAM  // Only printout when trying to autodetect
        printf("PSRAM not available\n");
    #endif
    }
#endif

#ifdef EXTRA_FUNC
    extern void EXTRA_FUNC(void);
    EXTRA_FUNC();
#endif

#ifndef __riscv
    exception_set_exclusive_handler(SVCALL_EXCEPTION, svc_call);
    // this should compile as we are Cortex-M
    pico_default_asm ("SVC #3");
#else
    exception_set_exclusive_handler(INSTR_ILLEGAL_EXCEPTION, svc_call);
    // this is an illegal instruction
    pico_default_asm (".word 0");
#endif
}
