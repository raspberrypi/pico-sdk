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
#include "hardware/rcp.h"

#include "hardware/structs/scb.h"
#include "hardware/structs/sau.h"
#include "hardware/structs/m33.h"
#include "hardware/structs/accessctrl.h"

#if PICO_SECURE

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

struct rom_secure_call_user_callback_slot {
    uint16_t fn_mask;
    rom_secure_call_callback_t callback;
} rom_secure_call_user_callback_slots[PICO_MAX_SECURE_CALL_USER_CALLBACKS];

int rom_secure_call_add_user_callback(rom_secure_call_callback_t callback, uint16_t fn_mask) {
    int first_unused = PICO_MAX_SECURE_CALL_USER_CALLBACKS;
    for (int i=0; i < PICO_MAX_SECURE_CALL_USER_CALLBACKS; i++) {
        if (!rom_secure_call_user_callback_slots[i].fn_mask) {
            if (first_unused == PICO_MAX_SECURE_CALL_USER_CALLBACKS) first_unused = i;
            continue;
        }

        // Check new function is not an existing function mask
        if (rom_secure_call_user_callback_slots[i].fn_mask == fn_mask) {
            return BOOTROM_ERROR_INVALID_ARG;
        }
    }

    if (first_unused == PICO_MAX_SECURE_CALL_USER_CALLBACKS) {
        // No free slots
        return BOOTROM_ERROR_BUFFER_TOO_SMALL;
    }

    rom_secure_call_user_callback_slots[first_unused].callback = callback;
    rom_secure_call_user_callback_slots[first_unused].fn_mask = fn_mask;

    return BOOTROM_OK;
}

void rom_secure_call_remove_user_callback(rom_secure_call_callback_t callback) {
    for (int i=0; i < PICO_MAX_SECURE_CALL_USER_CALLBACKS; i++) {
        if (rom_secure_call_user_callback_slots[i].callback == callback) {
            rom_secure_call_user_callback_slots[i].callback = NULL;
            rom_secure_call_user_callback_slots[i].fn_mask = 0;
            return;
        }
    }
}
#endif // PICO_SECURE


#if PICO_ALLOW_NONSECURE_STDIO
#include "pico/stdio/driver.h"

#if PICO_NONSECURE
static void stdio_nonsecure_out_chars(const char *buf, int length) {
    rom_secure_call((uint32_t)buf, length, 0, 0, SECURE_CALL_stdio_out_chars);
}

int stdio_nonsecure_in_chars(char *buf, int length) {
    return PICO_ERROR_NO_DATA;
}

static void stdio_nonsecure_out_flush(void) {}


stdio_driver_t stdio_nonsecure = {
    .out_chars = stdio_nonsecure_out_chars,
    .out_flush = stdio_nonsecure_out_flush,
    .in_chars = stdio_nonsecure_in_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = false, // CRLF is handled by the secure side
#endif
};

#if !PICO_RUNTIME_NO_INIT_NONSECURE_STDIO
void __weak runtime_init_nonsecure_stdio() {
    stdio_set_driver_enabled(&stdio_nonsecure, true);
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_STDIO
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_nonsecure_stdio, PICO_RUNTIME_INIT_NONSECURE_STDIO);
#endif

#endif // PICO_NONSECURE
#endif // PICO_ALLOW_NONSECURE_STDIO


#if PICO_ALLOW_NONSECURE_RAND
#include "pico/rand.h"

#if PICO_NONSECURE
// override the weak definition
uint64_t get_rand_64(void) {
    return rom_secure_call(0, 0, 0, 0, SECURE_CALL_get_rand_64);
}
#endif
#endif // PICO_ALLOW_NONSECURE_RAND


#if PICO_ALLOW_NONSECURE_DMA
#include "hardware/dma.h"

#if PICO_SECURE
static int dma_allocate_unused_channel_for_nonsecure(void) {
    int chan = dma_claim_unused_channel(false);
    if (chan < 0) return chan;
    if (chan > PICO_NONSECURE_DMA_MAX_CHANNEL) {
        dma_channel_unclaim(chan);
        return -1;
    }
    hw_clear_bits(&dma_hw->seccfg_ch[chan], DMA_SECCFG_CH0_S_BITS | DMA_SECCFG_CH0_LOCK_BITS);
    return chan;
}
#elif PICO_NONSECURE
int dma_request_unused_channels_from_secure(int num_channels) {
    int i;
    for (i = 0; i < num_channels; i++) {
        int chan = rom_secure_call(0, 0, 0, 0, SECURE_CALL_dma_allocate_unused_channel_for_nonsecure);
        if (chan < 0) break;
        dma_channel_unclaim(chan);
    }
    return i;
}
#endif
#endif // PICO_ALLOW_NONSECURE_DMA


#if PICO_ALLOW_NONSECURE_USER_IRQ
#include "hardware/irq.h"

#if PICO_SECURE
static int user_irq_claim_unused_for_nonsecure() {
    int bit = user_irq_claim_unused(false);
    if (bit < 0) return bit;
    irq_assign_to_ns(bit, true);
    return bit;
}
#elif PICO_NONSECURE
int user_irq_request_unused_from_secure(int num_irqs) {
    int i;
    for (i = 0; i < num_irqs; i++) {
        int irq = rom_secure_call(0, 0, 0, 0, SECURE_CALL_user_irq_claim_unused_for_nonsecure);
        if (irq < 0) break;
        user_irq_unclaim(irq);
    }
    return i;
}
#endif

#endif // PICO_ALLOW_NONSECURE_USER_IRQ


#if PICO_ALLOW_NONSECURE_PIO
#include "hardware/pio.h"
#include "hardware/irq.h"

#if PICO_SECURE
static int pio_claim_unused_pio_for_nonsecure(void) {
    // Find completely unused PIO
    uint pio;
    for (pio = 0; pio < PICO_NONSECURE_PIO_MAX; pio++) {
        // We need to claim an SM on the PIO
        int8_t sm_index[NUM_PIO_STATE_MACHINES];
        // on second pass, if there is one, we try and claim all the state machines so that we can change the GPIO base
        uint num_claimed;
        for(num_claimed = 0; num_claimed < NUM_PIO_STATE_MACHINES ; num_claimed++) {
            sm_index[num_claimed] = (int8_t)pio_claim_unused_sm(pio_get_instance(pio), false);
            if (sm_index[num_claimed] < 0) break;
        }

        if (num_claimed != NUM_PIO_STATE_MACHINES) {
            // un-claim all the SMs
            for (uint i = 0; i < num_claimed; i++) {
                pio_sm_unclaim(pio_get_instance(pio), (uint) sm_index[i]);
            }
            continue;
        }

        break;
    }
    
    if (pio == PICO_NONSECURE_PIO_MAX) {
        return -1;
    }

    // Accessctrl and IRQs
    accessctrl_hw->pio[pio] |= 0xacce0000 | ACCESSCTRL_PIO0_NSP_BITS | ACCESSCTRL_PIO0_NSU_BITS;

    static_assert(PIO0_IRQ_0 + 2 == PIO1_IRQ_0, "Expected 2 IRQs per PIO");

    irq_assign_to_ns(PIO0_IRQ_0 + pio * 2, true);
    irq_assign_to_ns(PIO0_IRQ_1 + pio * 2, true);

    return pio;
}
#elif PICO_NONSECURE
int pio_request_unused_pio_from_secure(void) {
    int pio = rom_secure_call(0, 0, 0, 0, SECURE_CALL_pio_claim_unused_pio_for_nonsecure);
    if (pio < 0) return pio;
    for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
        pio_sm_unclaim(pio_get_instance(pio), sm);
    }
    return pio;
}
#endif
#endif // PICO_ALLOW_NONSECURE_PIO


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

#if !PICO_RUNTIME_NO_INIT_BOOTROM_API_CALLBACK
#include <stdio.h>
#include "hardware/clocks.h"
#include "hardware/resets.h"
#include "hardware/structs/accessctrl.h"

int rom_default_secure_call_callback(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t fn) {
    if (fn >> 31) {
        // User callbacks all start with 0b1xxx, as specified by the rom_secure_call() documentation
        for (int i=0; i < PICO_MAX_SECURE_CALL_USER_CALLBACKS; i++) {
            if ((fn >> 16) == rom_secure_call_user_callback_slots[i].fn_mask) {
                return rom_secure_call_user_callback_slots[i].callback(a, b, c, d, fn);
            }
        }

        return BOOTROM_ERROR_INVALID_ARG;
    }

    switch (fn) {
    #if PICO_ALLOW_NONSECURE_STDIO
        case SECURE_CALL_stdio_out_chars: {
            uint32_t ok = RCP_MASK_FALSE;
            rom_validate_ns_buffer((char*)a, b, RCP_MASK_TRUE, &ok);
            if (ok != RCP_MASK_TRUE) return BOOTROM_ERROR_NOT_PERMITTED;
            stdio_put_string((char*)a, b, false, true);
            stdio_flush();
            return BOOTROM_OK;
        }
    #endif
    #if PICO_ALLOW_NONSECURE_RAND
        case SECURE_CALL_get_rand_64: {
            return get_rand_64();
        }
    #endif
    #if PICO_ALLOW_NONSECURE_DMA
        case SECURE_CALL_dma_allocate_unused_channel_for_nonsecure: {
            return dma_allocate_unused_channel_for_nonsecure();
        }
    #endif
    #if PICO_ALLOW_NONSECURE_USER_IRQ
        case SECURE_CALL_user_irq_claim_unused_for_nonsecure: {
            return user_irq_claim_unused_for_nonsecure();
        }
    #endif
    #if PICO_ALLOW_NONSECURE_PIO
        case SECURE_CALL_pio_claim_unused_pio_for_nonsecure: {
            return pio_claim_unused_pio_for_nonsecure();
        }
    #endif
    #if PICO_ADD_NONSECURE_PADS_HELPER
        case SECURE_CALL_pads_bank0_set_bits: {
            if (accessctrl_hw->gpio_nsmask[a/32] & 1u << (a & 0x1fu)) {
                pads_bank0_set_bits(a, b);
                return BOOTROM_OK;
            } else {
                return BOOTROM_ERROR_NOT_PERMITTED;
            }
        }
        case SECURE_CALL_pads_bank0_clear_bits: {
            if (accessctrl_hw->gpio_nsmask[a/32] & 1u << (a & 0x1fu)) {
                pads_bank0_clear_bits(a, b);
                return BOOTROM_OK;
            } else {
                return BOOTROM_ERROR_NOT_PERMITTED;
            }
        }
        case SECURE_CALL_pads_bank0_write_masked: {
            if (accessctrl_hw->gpio_nsmask[a/32] & 1u << (a & 0x1fu)) {
                pads_bank0_write_masked(a, b, c);
                return BOOTROM_OK;
            } else {
                return BOOTROM_ERROR_NOT_PERMITTED;
            }
        }
        case SECURE_CALL_pads_bank0_read: {
            if (accessctrl_hw->gpio_nsmask[a/32] & 1u << (a & 0x1fu)) {
                return pads_bank0_read(a);
            } else {
                return BOOTROM_ERROR_NOT_PERMITTED;
            }
        }
    #endif
        case SECURE_CALL_clock_get_hz: {
            return clock_get_hz(a);
        }
    #if PICO_ALLOW_NONSECURE_RESETS
        case SECURE_CALL_reset_block_mask: {
            if (a & ~PICO_ALLOW_NONSECURE_RESETS_MASK) return BOOTROM_ERROR_NOT_PERMITTED;
            reset_block_mask(a);
            return BOOTROM_OK;
        }
        case SECURE_CALL_unreset_block_mask: {
            if (a & ~PICO_ALLOW_NONSECURE_RESETS_MASK) return BOOTROM_ERROR_NOT_PERMITTED;
            unreset_block_mask(a);
            return BOOTROM_OK;
        }
        case SECURE_CALL_unreset_block_mask_wait_blocking: {
            if (a & ~PICO_ALLOW_NONSECURE_RESETS_MASK) return BOOTROM_ERROR_NOT_PERMITTED;
            unreset_block_mask_wait_blocking(a);
            return BOOTROM_OK;
        }
    #endif
        default: {
            printf("%08x is not a supported rom function\n", fn);
            return BOOTROM_ERROR_INVALID_ARG;
        }

    }
}

static int __attribute__((naked)) rom_default_asm_callback() {
    pico_default_asm_volatile(
        "push {r0, lr}\n"
        "str r4, [sp]\n"
        "bl rom_default_secure_call_callback\n"
        "pop {r1, pc}\n"
    );
}

void __weak runtime_init_rom_set_default_callback() {
    rom_set_rom_callback(BOOTROM_API_CALLBACK_secure_call, (bootrom_api_callback_generic_t) rom_default_asm_callback);

    rom_set_ns_api_permission(BOOTROM_NS_API_secure_call, true);
}
#endif // !PICO_RUNTIME_NO_INIT_BOOTROM_API_CALLBACK

#if !PICO_RUNTIME_SKIP_INIT_BOOTROM_API_CALLBACK
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_rom_set_default_callback, PICO_RUNTIME_INIT_BOOTROM_API_CALLBACK);
#endif

#if !PICO_RUNTIME_NO_INIT_NONSECURE_CLAIMS
void __weak runtime_init_nonsecure_claims() {
#if PICO_ALLOW_NONSECURE_DMA
    for(uint i = 0; i < NUM_DMA_CHANNELS; i++) {
        dma_channel_claim(i);
    }
#endif
#if PICO_ALLOW_NONSECURE_USER_IRQ
    for (uint i = 0; i < NUM_USER_IRQS; i++) {
        user_irq_claim(FIRST_USER_IRQ + i);
    }
#endif
#if PICO_ALLOW_NONSECURE_PIO
    for (uint pio = 0; pio < NUM_PIOS; pio++) {
        for (uint sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
            pio_sm_claim(pio_get_instance(pio), sm);
        }
    }
#endif
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_CLAIMS
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_nonsecure_claims, PICO_RUNTIME_INIT_NONSECURE_CLAIMS);
#endif

#if !PICO_RUNTIME_NO_INIT_NONSECURE_CLOCKS
#include "hardware/clocks.h"

void __weak runtime_init_nonsecure_clocks() {
    // Set all clocks to the reported frequency from the secure side
    for (uint i = 0; i < CLK_COUNT; i++) {
        uint32_t hz = rom_secure_call(i, 0, 0, 0, SECURE_CALL_clock_get_hz);
        clock_set_reported_hz(i, hz);
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_CLOCKS
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_nonsecure_clocks, PICO_RUNTIME_INIT_NONSECURE_CLOCKS);
#endif
