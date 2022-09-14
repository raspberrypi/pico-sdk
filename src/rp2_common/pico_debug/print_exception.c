/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdio.h"

#include "hardware/address_mapped.h"
#include "hardware/structs/mpu.h"

#include <stdio.h>

struct hw_saved_regs_t {
    // These are volatile because want to verify their addresses before accessing them.
    volatile uint r0;
    volatile uint r1;
    volatile uint r2;
    volatile uint r3;
    volatile uint r12;
    volatile uint lr;
    volatile uint pc;
    volatile uint xpsr;
};

struct sw_saved_regs_t {
    uint r8;
    uint r9;
    uint r10;
    uint r11;
    uint primask;
    uint psp;
    uint msp;
    uint r4;
    uint r5;
    uint r6;
    uint r7;
};

static const char* const exception_names[] = {
    0,
    "Reset",
    "NMI",
    "Hard Fault",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    "SVCall",
    0,
    0,
    "PendSV",
    "SysTick",

    "TIMER_IRQ_0",  // 16
    "TIMER_IRQ_1",
    "TIMER_IRQ_2",
    "TIMER_IRQ_3",
    "PWM_IRQ_WRAP",
    "USBCTRL_IRQ",
    "XIP_IRQ",
    "PIO0_IRQ_0",
    "PIO0_IRQ_1",
    "PIO1_IRQ_0",
    "PIO1_IRQ_1",
    "DMA_IRQ_0",
    "DMA_IRQ_1",
    "IO_IRQ_BANK0",
    "IO_IRQ_QSPI",
    "SIO_IRQ_PROC0",

    "SIO_IRQ_PROC1",  // 32
    "CLOCKS_IRQ",
    "SPI0_IRQ",
    "SPI1_IRQ",
    "UART0_IRQ",
    "UART1_IRQ",
    "ADC_IRQ_FIFO",
    "I2C0_IRQ",
    "I2C1_IRQ",
    "RTC_IRQ",
    "USER_IRQ_0",
    "USER_IRQ_1",
    "USER_IRQ_2",
    "USER_IRQ_3",
    "USER_IRQ_4",
    "USER_IRQ_5",
};

static_assert(count_of(exception_names) == 16 + 32);

static bool verify_stack_address(uint address) {
    if ((address & 3) != 0) return false;

    // Check SP falls in SRAM banks 0-3, either the striped or unstriped aliases, or in SRAM banks 4 or 5.
    return (address >= SRAM_BASE && address < SRAM_END) || (address >= SRAM0_BASE && address < (SRAM3_BASE + 0x10000));
}

static uint safe_read_stack(const volatile uint* address) {
    if (verify_stack_address((uint) address)) {
        return *address;
    } else {
        return 0;
    }
}

void debug_print_exception(uint exc_return, uint ipsr, const struct sw_saved_regs_t* sw_saved_regs) {
    static const char* const separator = "***********************************************************\r\n";

    // Stack bounds are unknown - an RTOS might have put it anywhere in static RAM - so there is a danger of accidentally hitting an
    // MPU region. It might be worth verifying stack addresses against each enabled MPU region but not clear why. Until someone
    // complains, just disable the MPU.
    mpu_hw->ctrl = 0;

    printf("\r\n");
    printf(separator);
    printf("Unhandled %s exception on core %d\r\n", exception_names[ipsr], get_core_num());
    printf(separator);
    
    // Recover registers saved by hardware and determine stack pointer register values
    // prior to exception entry.
    uint msp = sw_saved_regs->msp;
    uint psp = sw_saved_regs->psp;
    uint sp;
    const struct hw_saved_regs_t* hw_saved_regs;
    if ((exc_return & 0xF) == 0xD) {
        // Be RTOS friendly by checking to see if state was saved on the process stack rather than the main stack.
        hw_saved_regs = (const struct hw_saved_regs_t*) psp;
        psp += sizeof(*hw_saved_regs);
        sp = psp;
    } else {
        hw_saved_regs = (const struct hw_saved_regs_t*) msp;
        msp += sizeof(*hw_saved_regs);
        sp = msp;
    }

    printf("  R0: %08x   R1: %08x   R2: %08x   R3: %08x\r\n", safe_read_stack(&hw_saved_regs->r0), safe_read_stack(&hw_saved_regs->r1),
                                                              safe_read_stack(&hw_saved_regs->r2), safe_read_stack(&hw_saved_regs->r3));
    printf("  R4: %08x   R5: %08x   R6: %08x   R7: %08x\r\n", sw_saved_regs->r4, sw_saved_regs->r5, sw_saved_regs->r6, sw_saved_regs->r7);
    printf("  R8: %08x   R9: %08x  R10: %08x  R11: %08x\r\n", sw_saved_regs->r8, sw_saved_regs->r9, sw_saved_regs->r10, sw_saved_regs->r11);
    printf(" R12: %08x   SP: %08x   LR: %08x   PC: %08x\r\n", safe_read_stack(&hw_saved_regs->r12), sp,
                                                              safe_read_stack(&hw_saved_regs->lr), safe_read_stack(&hw_saved_regs->pc));
    printf("               XPSR: %08x  PSP: %08x  MSP: %08x\r\n", safe_read_stack(&hw_saved_regs->xpsr), psp, msp);
    printf("         EXC_RETURN: %08x             PRIMASK: %08x\r\n", exc_return, sw_saved_regs->primask);
    printf(separator);

    for (int i = 0; i < 32; i += 4) {
        printf("%08x:", sp);
        for (int j = 0; j < 4; ++j) {
            if (verify_stack_address(sp)) {
                printf(" %08x", *(uint*)sp);
            } else {
                printf(" BAD ADDR");
            }
            sp += 4;
        }
        printf("\r\n");
    }
}
