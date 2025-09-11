#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/busctrl.h"


int __time_critical_func(test_func)(void) {
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


int main(void) {
    stdio_init_all();
    printf("pico_xip_sram_test begins\n");

    multicore_launch_core1(core1_entry);

    systick_hw->csr = 0x4 | 0x1; // clock source and enable

    // Give core1 high priority
    hw_set_bits(&busctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);

    for (int i = 0; i < 5; i++) {
        printf("running... %d\n", i);
        printf("test_func: %d\n", test_func());
        sleep_ms(500);
    }

    printf("pico_xip_sram_test ends\n");
    return 0;
}
