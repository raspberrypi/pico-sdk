#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/test.h"
#include "pico/multicore.h"
#include "hardware/dma.h"
#if PICO_RP2040
#include "hardware/structs/systick.h"
#else
#include "hardware/riscv_platform_timer.h"
#endif
#include "hardware/structs/busctrl.h"

#if LIB_PICO_LOW_POWER
int __persistent_data(some_data);
#endif


PICOTEST_MODULE_NAME("XIP_SRAM", "critical xip sram test");


int __time_critical_func(test_func_xip)(void) {
#if PICO_RP2040
    systick_hw->rvr = 0x00ffffff;
    systick_hw->cvr = 0;
#else
    riscv_timer_set_mtimecmp(0xffffffffffffffff);
    riscv_timer_set_mtime(0);
#endif

    volatile uint32_t i = 0;
    i += 4;
    i += i;
    i += 7;
    i += i;
    i += i;

#if PICO_RP2040
    return systick_hw->rvr - systick_hw->cvr;
#else
    return riscv_timer_get_mtime();
#endif
}

int __no_inline_not_in_flash_func(test_func_sram)(void) {
#if PICO_RP2040
    systick_hw->rvr = 0x00ffffff;
    systick_hw->cvr = 0;
#else
    riscv_timer_set_mtimecmp(0xffffffffffffffff);
    riscv_timer_set_mtime(0);
#endif

    volatile uint32_t i = 0;
    i += 4;
    i += i;
    i += 7;
    i += i;
    i += i;

#if PICO_RP2040
    return systick_hw->rvr - systick_hw->cvr;
#else
    return riscv_timer_get_mtime();
#endif
}


void core1_entry() {
    // Just read memory from SRAM bank 0 repeatedly
    // word-striped across 4 banks, so read every 16 bytes
#ifndef __riscv
    pico_default_asm_volatile(
        "1:\n"
        "ldr r0, =%c0\n"
        "ldr r1, [r0, #0]\n"
        "ldr r2, [r0, #16]\n"
        "ldr r3, [r0, #32]\n"
        "ldr r4, [r0, #48]\n"
        "adds r0, #64\n"
        "ldr r1, [r0, #0]\n"
        "ldr r2, [r0, #16]\n"
        "ldr r3, [r0, #32]\n"
        "ldr r4, [r0, #48]\n"
        "adds r0, #64\n"
        "ldr r1, [r0, #0]\n"
        "ldr r2, [r0, #16]\n"
        "ldr r3, [r0, #32]\n"
        "ldr r4, [r0, #48]\n"
        "b 1b\n"
        : : "i" (SRAM_BASE) : "r0", "r1", "r2", "r3", "r4"
    );
#else
    pico_default_asm_volatile(
        "1:\n"
        "li a0, %0\n"
        "lw a1, 0(a0)\n"
        "lw a2, 16(a0)\n"
        "lw a3, 32(a0)\n"
        "lw a4, 48(a0)\n"
        "addi a0, a0, 64\n"
        "lw a1, 0(a0)\n"
        "lw a2, 16(a0)\n"
        "lw a3, 32(a0)\n"
        "lw a4, 48(a0)\n"
        "addi a0, a0, 64\n"
        "lw a1, 0(a0)\n"
        "lw a2, 16(a0)\n"
        "lw a3, 32(a0)\n"
        "lw a4, 48(a0)\n"
        "j 1b\n"
        : : "i" (SRAM_BASE) : "a0", "a1", "a2", "a3", "a4"
    );
#endif
}


void trigger_dma(void) {
    int dat[8];
    for (int i = 0; i < count_of(dat); i++) {
        int chan = dma_claim_unused_channel(true);
        dma_channel_config c = dma_channel_get_default_config(chan);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, true);
        uint32_t from = SRAM_BASE;
        uint32_t to = SRAM_BASE + ((SRAM4_BASE - SRAM_BASE) / 2);
        uint32_t size = ((SRAM4_BASE - SRAM_BASE) / 2) / 4;
        dma_channel_configure(chan, &c, (uint32_t*)to, (uint32_t*)from, dma_encode_transfer_count(size), true);
        dat[i] = chan;
    }
    for (int i = 0; i < count_of(dat); i++) {
        dma_channel_unclaim(dat[i]);
    }
}


#ifndef TEXT_FUNC_XIP_BASE
#define TEXT_FUNC_XIP_BASE XIP_SRAM_BASE
#endif

#ifndef TEXT_FUNC_XIP_END
#define TEXT_FUNC_XIP_END XIP_SRAM_END
#endif

#ifndef TEXT_FUNC_SRAM_BASE
#define TEXT_FUNC_SRAM_BASE SRAM_STRIPED_BASE
#endif

#ifndef TEXT_FUNC_SRAM_END
#define TEXT_FUNC_SRAM_END SRAM4_BASE
#endif


int main(void) {
    stdio_init_all();
    printf("pico_xip_sram_test begins\n");

    PICOTEST_START();

    #if LIB_PICO_LOW_POWER
    PICOTEST_START_SECTION("test_low_power")
    PICOTEST_CHECK(some_data == 0, "persistent some_data is not 0");
    PICOTEST_END_SECTION()
    #endif
    
    PICOTEST_START_SECTION("test_func_addresses")
    printf("test_func_xip at %p\n", test_func_xip);
    printf("test_func_sram at %p\n", test_func_sram);

    PICOTEST_CHECK((uint32_t)test_func_xip >= TEXT_FUNC_XIP_BASE && (uint32_t)test_func_xip < TEXT_FUNC_XIP_END, "test_func_xip is not in XIP SRAM");
    PICOTEST_CHECK((uint32_t)test_func_sram >= TEXT_FUNC_SRAM_BASE && (uint32_t)test_func_sram < TEXT_FUNC_SRAM_END, "test_func_sram is not in SRAM 0-3");
    PICOTEST_END_SECTION()

    multicore_launch_core1(core1_entry);

#if PICO_RP2040
    systick_hw->csr = 0x4 | 0x1; // clock source and enable
#else
    riscv_timer_set_fullspeed(true);
    riscv_timer_set_enabled(true);
    riscv_timer_set_mtimecmp(0xffffffffffffffff);
#endif

    // Give core1 and DMA high priority
    hw_set_bits(&busctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS);
    hw_clear_bits(&busctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC0_BITS);

    PICOTEST_START_SECTION("test_func");
    int test_func_xip_cycles = 0;
    int test_func_sram_cycles = 0;
    for (int i = 0; i < 5; i++) {
        printf("running... %d\n", i);
        trigger_dma();
        int tmp = test_func_xip();
        test_func_xip_cycles += tmp;
        printf("test_func_xip: %d\n", tmp);
        trigger_dma();
        tmp = test_func_sram();
        test_func_sram_cycles += tmp;
        printf("test_func_sram: %d\n", tmp);
        sleep_ms(500);
    }

#if TEXT_FUNC_SRAM_BASE == SRAM_STRIPED_BASE
    PICOTEST_CHECK(test_func_xip_cycles < test_func_sram_cycles, "test_func_xip took longer than test_func_sram");
#endif

    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}
