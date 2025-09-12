#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/dma.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/busctrl.h"


int __time_critical_func(test_func_xip)(void) {
    systick_hw->rvr = 0x00ffffff;
    systick_hw->cvr = 0;

    volatile uint32_t i = 0;
    i += 4;
    i += i;

    return systick_hw->rvr - systick_hw->cvr;
}

int __not_in_flash_func(test_func_sram)(void) {
    systick_hw->rvr = 0x00ffffff;
    systick_hw->cvr = 0;

    volatile uint32_t i = 0;
    i += 4;
    i += i;

    return systick_hw->rvr - systick_hw->cvr;
}


void core1_entry() {
    // Just read memory repeatedly
    pico_default_asm_volatile(
        "1:\n"
        "ldr r0, =%0\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "ldmia r0!, {r1-r4}\n"
        "b 1b\n"
        : : "i" (SRAM_BASE) : "r0", "r1", "r2", "r3", "r4"
    );
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
        dma_channel_configure(chan, &c, (uint32_t*)to, (uint32_t*)from, size, true);
        dat[i] = chan;
    }
    for (int i = 0; i < count_of(dat); i++) {
        dma_channel_unclaim(dat[i]);
    }
}


int main(void) {
    stdio_init_all();
    printf("pico_xip_sram_test begins\n");

    multicore_launch_core1(core1_entry);

    systick_hw->csr = 0x4 | 0x1; // clock source and enable

    // Give core1 and DMA high priority
    hw_set_bits(&busctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS);
    hw_clear_bits(&busctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC0_BITS);

    int test_func_xip_cycles = 0;
    int test_func_sram_cycles = 0;
    for (int i = 0; i < 5; i++) {
        printf("running... %d\n", i);
        trigger_dma();
        int tmp = test_func_xip();
        test_func_xip_cycles += tmp;
        printf("test_func_xip: %d\n", tmp);
        tmp = test_func_sram();
        test_func_sram_cycles += tmp;
        printf("test_func_sram: %d\n", tmp);
        sleep_ms(500);
    }

    if (test_func_xip_cycles >= test_func_sram_cycles) {
        printf("ERROR: test_func_xip_cycles (%d) >= test_func_sram_cycles (%d)\n", test_func_xip_cycles, test_func_sram_cycles);
        return 1;
    } else {
        printf("SUCCESS: test_func_xip_cycles (%d) < test_func_sram_cycles (%d)\n", test_func_xip_cycles, test_func_sram_cycles);
    }

    printf("pico_xip_sram_test ends\n");
    return 0;
}
