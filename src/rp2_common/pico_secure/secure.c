/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/secure.h"
#include "pico/runtime_init.h"

#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/exception.h"

#include "hardware/structs/scb.h"
#include "hardware/structs/sau.h"
#include "hardware/structs/m33.h"
#include "hardware/structs/accessctrl.h"

void __attribute__((noreturn)) secure_launch_nonsecure_binary(uint32_t vtor_address, uint32_t stack_limit) {
    uint32_t *vtor = (uint32_t*)vtor_address;
    uint32_t stack_pointer = *(vtor + 0);
    uint32_t entry_point = *(vtor + 1);
    scb_ns_hw->vtor = vtor_address;

    pico_default_asm(
        "msr msp_ns, %0\n"
        "msr msplim_ns, %1\n"
        "movs r1, %2\n"
        "bxns r1"
        :
        :   "r" (stack_pointer),
            "r" (stack_limit),
            "r" (entry_point & ~1)  // make sure thumb bit is clear for blxns
    );

    __builtin_unreachable();
}


void secure_sau_configure_region(uint region, uint32_t base, uint32_t limit, bool enabled, bool nsc) {
    sau_hw->rnr = region;
    sau_hw->rbar = base & M33_SAU_RBAR_BADDR_BITS;
    sau_hw->rlar = ((limit-1) & M33_SAU_RLAR_LADDR_BITS) | (nsc ? M33_SAU_RLAR_NSC_BITS : 0) | (enabled ? M33_SAU_RLAR_ENABLE_BITS : 0);
}


void secure_sau_set_enabled(bool enabled) {
    uint32_t save = save_and_disable_interrupts();
    __dmb();

    if (enabled)
        sau_hw->ctrl |= M33_SAU_CTRL_ENABLE_BITS;
    else
        sau_hw->ctrl &= ~M33_SAU_CTRL_ENABLE_BITS;

    __dsb();
    __isb();
    restore_interrupts_from_disabled(save);
}


#if defined(PICO_SECURITY_SPLIT_CONFIGURED)
static uint32_t nonsecure_ram_start = 0;

void secure_sau_configure_split() {
#if !PICO_SECURITY_SPLIT_NO_FLASH
    // XIP is NS Code
    secure_sau_configure_region(0, XIP_BASE, XIP_END, true, false);
#endif

#if defined(PICO_SECURITY_SPLIT_SIMPLE)
    // SRAM after secure stack is NS data
    extern uint32_t __StackTop;
    secure_sau_configure_region(1, (uint32_t)&__StackTop, SRAM_END, true, false);
    nonsecure_ram_start = (uint32_t)&__StackTop;
#elif defined(PICO_SECURITY_SPLIT_SCRATCH_EACH)
    // Main SRAM after secure scratch X is NS data
    extern uint32_t __StackOneTop;
    secure_sau_configure_region(1, (uint32_t)&__StackOneTop, SRAM_STRIPED_END, true, false);
    nonsecure_ram_start = (uint32_t)&__StackOneTop;
    // Scratch after secure stack in NS stack
    extern uint32_t __StackTop;
    secure_sau_configure_region(2, (uint32_t)&__StackTop, SRAM_END, true, false);
#elif defined(PICO_SECURITY_SPLIT_SECURE_SCRATCH)
    // Main SRAM after secure heap is NS data
    extern uint32_t __HeapLimit;
    secure_sau_configure_region(1, (uint32_t)&__HeapLimit, SRAM_STRIPED_END, true, false);
    nonsecure_ram_start = (uint32_t)&__HeapLimit;
#endif
}


void __attribute__((noreturn)) secure_launch_nonsecure_binary_default() {
#if PICO_SECURITY_SPLIT_NO_FLASH
    uint32_t nonsecure_vtor = nonsecure_ram_start;
#else
    uint32_t nonsecure_vtor = XIP_BASE;
#endif

#if defined(PICO_SECURITY_SPLIT_SIMPLE)
    // Nonsecure running from XIP, stack limit is bottom of scratch
    secure_launch_nonsecure_binary(nonsecure_vtor, SRAM_SCRATCH_X_BASE);
#elif defined(PICO_SECURITY_SPLIT_SCRATCH_EACH)
    // Nonsecure running from XIP, stack limit is bottom of scratch Y
    secure_launch_nonsecure_binary(nonsecure_vtor, SRAM_SCRATCH_Y_BASE);
#elif defined(PICO_SECURITY_SPLIT_SECURE_SCRATCH)
    // Nonsecure running from XIP, stack limit is secure heap limit
    extern uint32_t __HeapLimit;
    secure_launch_nonsecure_binary(nonsecure_vtor, (uint32_t)&__HeapLimit);
#endif
}
#endif


static secure_hardfault_callback_t hardfault_callback = NULL;


static void secure_hardfault_handler(void) {
    printf("Hard fault occurred\n");

    // # First eight values on stack will always be:
    // # r0, r1, r2, r3, r12, LR, pc, xPSR

    uint32_t sp;
    pico_default_asm_volatile(
        "mrs %0, msp_ns"
        : "=r" (sp)
    );

    printf("sp:   %08x\n", sp);
    printf("r0:   %08x\n", *((uint32_t*)sp + 0));
    printf("r1:   %08x\n", *((uint32_t*)sp + 1));
    printf("r2:   %08x\n", *((uint32_t*)sp + 2));
    printf("r3:   %08x\n", *((uint32_t*)sp + 3));
    printf("r12:  %08x\n", *((uint32_t*)sp + 4));
    printf("lr:   %08x\n", *((uint32_t*)sp + 5));
    printf("pc:   %08x\n", *((uint32_t*)sp + 6));
    printf("xPSR: %08x\n", *((uint32_t*)sp + 7));

    if (scb_hw->hfsr & M33_HFSR_DEBUGEVT_BITS) printf("HardFault: Debug Event\n");
    if (scb_hw->hfsr & M33_HFSR_FORCED_BITS) printf("HardFault: Forced\n");
    if (scb_hw->hfsr & M33_HFSR_VECTTBL_BITS) printf("HardFault: Vector Table Read Error\n");

    if (m33_hw->sfsr & M33_SFSR_LSERR_BITS) printf("SecureFault: Error occurred during lazy state activation/deactivation\n");
    if (m33_hw->sfsr & M33_SFSR_LSPERR_BITS) printf("SecureFault: Error occurred during lazy preservation of floating-point state\n");
    if (m33_hw->sfsr & M33_SFSR_INVTRAN_BITS) printf("SecureFault: Secure branched to Non-secure code\n");
    if (m33_hw->sfsr & M33_SFSR_AUVIOL_BITS) printf("SecureFault: Non-secure accessed Secure memory\n");
    if (m33_hw->sfsr & M33_SFSR_INVER_BITS) printf("SecureFault: Invalid Non-secure exception state when returning\n");
    if (m33_hw->sfsr & M33_SFSR_INVIS_BITS) printf("SecureFault: Invalid integrity signature in exception stack\n");
    if (m33_hw->sfsr & M33_SFSR_INVEP_BITS) printf("SecureFault: Non-secure branched to Secure code\n");
    if (m33_hw->sfsr & M33_SFSR_SFARVALID_BITS) printf("SecureFault address: %08x\n", m33_hw->sfar);

    if (scb_hw->cfsr & M33_CFSR_UFSR_DIVBYZERO_BITS) printf("UsageFault: Division by zero\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_UNALIGNED_BITS) printf("UsageFault: Unaligned access\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_STKOF_BITS) printf("UsageFault: Stack overflow\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_NOCP_BITS) printf("UsageFault: No Coprocessor\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_INVPC_BITS) printf("UsageFault: Invalid PC\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_INVSTATE_BITS) printf("UsageFault: Invalid state\n");
    if (scb_hw->cfsr & M33_CFSR_UFSR_UNDEFINSTR_BITS) printf("UsageFault: Undefined instruction\n");

    if (scb_hw->cfsr & M33_CFSR_BFSR_LSPERR_BITS) printf("BusFault: Error occurred during lazy preservation of floating-point state\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_STKERR_BITS) printf("BusFault: Error occurred during exception entry stacking\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_UNSTKERR_BITS) printf("BusFault: Error occurred during exception return unstacking\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_IMPRECISERR_BITS) printf("BusFault: Imprecise data access error\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_PRECISERR_BITS) printf("BusFault: Precise data access error\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_IBUSERR_BITS) printf("BusFault: Bus fault on instruction prefetch\n");
    if (scb_hw->cfsr & M33_CFSR_BFSR_BFARVALID_BITS) printf("BusFault address: %08x\n", scb_hw->bfar);

    if (scb_hw->cfsr & M33_CFSR_MMFSR_BITS) printf("MemManageFault: %02x\n", scb_hw->cfsr & M33_CFSR_MMFSR_BITS);
    if (scb_hw->cfsr & 0x80) printf("MemManageFault address: %08x\n", scb_hw->mmfar);

    if (hardfault_callback) {
        hardfault_callback();
    }
}


void secure_install_default_hardfault_handler(secure_hardfault_callback_t callback) {
    hardfault_callback = callback;
    exception_set_exclusive_handler(HARDFAULT_EXCEPTION, secure_hardfault_handler);
}


#if !PICO_RUNTIME_NO_INIT_NONSECURE_COPROCESSORS
void __weak runtime_init_nonsecure_coprocessors() {
    // Enable NS coprocessor access to anything secure has enabled
    uint32_t cpacr = arm_cpu_hw->cpacr;
    uint32_t nsacr = 0;
    for (int i = 0; i < 16; i++) {
        if (cpacr & (M33_CPACR_CP0_BITS << (i * M33_CPACR_CP1_LSB))) {
            nsacr |= (0x1 << i);
        }
    }
    arm_cpu_hw->nsacr |= nsacr;
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_COPROCESSORS
PICO_RUNTIME_INIT_FUNC_PER_CORE(runtime_init_nonsecure_coprocessors, PICO_RUNTIME_INIT_NONSECURE_COPROCESSORS);
#endif


#if !PICO_RUNTIME_NO_INIT_NONSECURE_ACCESSCTRL_AND_IRQS
void __weak runtime_init_nonsecure_accessctrl_and_irqs() {
    rom_set_ns_api_permission(BOOTROM_NS_API_get_sys_info, true);

    #if PICO_ALLOW_NONSECURE_DMA
        accessctrl_hw->dma |= 0xacce0000 | ACCESSCTRL_DMA_NSP_BITS | ACCESSCTRL_DMA_NSU_BITS;
    #endif

    #ifdef PICO_ASSIGN_NONSECURE_TIMER
        accessctrl_hw->timer[PICO_ASSIGN_NONSECURE_TIMER] |= 0xacce0000 | ACCESSCTRL_TIMER0_NSP_BITS | ACCESSCTRL_TIMER0_NSU_BITS;

        static_assert(TIMER0_IRQ_0 + 4 == TIMER1_IRQ_0, "Expected 4 IRQs per TIMER");

        irq_assign_to_ns(TIMER0_IRQ_0 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_1 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_2 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_3 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
    #endif

    #if PICO_ALLOW_NONSECURE_GPIO
        accessctrl_hw->io_bank[0] |= 0xacce0000 | ACCESSCTRL_IO_BANK0_NSP_BITS | ACCESSCTRL_IO_BANK0_NSU_BITS;

        irq_assign_to_ns(IO_IRQ_BANK0_NS, true);
    #endif

    #if PICO_ALLOW_NONSECURE_USB
        accessctrl_hw->usbctrl |= 0xacce0000 | ACCESSCTRL_USBCTRL_NSP_BITS | ACCESSCTRL_USBCTRL_NSU_BITS;

        irq_assign_to_ns(USBCTRL_IRQ, true);
    #endif
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_ACCESSCTRL_AND_IRQS
PICO_RUNTIME_INIT_FUNC_HW(runtime_init_nonsecure_accessctrl_and_irqs, PICO_RUNTIME_INIT_NONSECURE_ACCESSCTRL_AND_IRQS);
#endif
