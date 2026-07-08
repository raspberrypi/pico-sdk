/*
* Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_PLATFORM_COMMON_H
#define _PICO_PLATFORM_COMMON_H

/** \file pico/platform/common.h
 *  \ingroup pico_platform
 *
 * \brief Macros and definitions common to all rp2 platforms but not specific to any library
 *
 * This header may be included by assembly code
 *
 * Note certain library specific defines are defined here when they are interdpedent across libraries,
 * but making an explicit library dependency does not make sense.
 */

// PICO_CONFIG: PICO_MINIMAL_STORED_VECTOR_TABLE, Only store a very minimal vector table in the binary on Arm, type=bool, default=0, advanced=true, group=pico_crt0
#ifndef PICO_MINIMAL_STORED_VECTOR_TABLE
#define PICO_MINIMAL_STORED_VECTOR_TABLE 0
#endif

#if PICO_MINIMAL_STORED_VECTOR_TABLE && (PICO_NO_FLASH && !defined(__riscv))
#if PICO_NUM_VTABLE_IRQS
#warning PICO_NUM_VTABLE_IRQS is specied with PICO_MINIMAL_STORED_VECTOR_TABLE for NO_FLASH Arm binary; ignored
#undef PICO_NUM_VTABLE_IRQS
#endif
#define PICO_NUM_VTABLE_IRQS 0
#else
// PICO_CONFIG: PICO_NUM_VTABLE_IRQS, Number of IRQ handlers in the vector table - can be lowered to save space if you aren't using some higher IRQs, type=int, default=NUM_IRQS, group=hardware_irq
#ifndef PICO_NUM_VTABLE_IRQS
#define PICO_NUM_VTABLE_IRQS NUM_IRQS
#endif
#endif

// note that this is not a safely overridable value, you should use override PICO_NUM_VTABLE_IRQs instead.
// keeping around as a #define though as it used to be supported
#ifdef PICO_RAM_VECTOR_TABLE_SIZE
#warning Overriding PICO_RAM_VECTOR_TABLE_SIZE is deprecated; specify PICO_NUM_VTABLE_IRQS instead
#endif
#ifndef PICO_RAM_VECTOR_TABLE_SIZE
#define PICO_RAM_VECTOR_TABLE_SIZE (VTABLE_FIRST_IRQ + PICO_NUM_VTABLE_IRQS)
#endif

#ifndef PICO_RAM_VECTOR_TABLE_ALIGNMENT
#if PICO_RAM_VECTOR_TABLE_SIZE <= 64
#define PICO_RAM_VECTOR_TABLE_ALIGNMENT 256
#define PICO_RAM_VECTOR_TABLE_P2ALIGNMENT 8
#elif PICO_RAM_VECTOR_TABLE_SIZE <= 128
#define PICO_RAM_VECTOR_TABLE_ALIGNMENT 512
#define PICO_RAM_VECTOR_TABLE_P2ALIGNMENT 9
#else
// crt0.S only supports 80 IRQs at the moment anyway, giving max size of (16 + 80) = 96
#error "Need to add PICO_RAM_VECTOR_TABLE_ALIGNMENT defines for PICO_RAM_VECTOR_TABLE_SIZE > 128"
#endif
#endif

// PICO_CONFIG: PICO_VTABLE_PER_CORE, Use separate vector tables per core, type=bool, default=0, group=hardware_irq
#ifndef PICO_VTABLE_PER_CORE
#define PICO_VTABLE_PER_CORE 0
#endif

// PICO_CONFIG: PICO_CORE1_VTABLE_PLACEMENT, Placement macro for the core 1 vector table on Arm (ignored on RISC-V), type=string, default=__in_data for no_flash binary types, __in_bss otherwise, group=hardware_irq
#ifndef PICO_CORE1_VTABLE_PLACEMENT
#if PICO_NO_FLASH
#define PICO_CORE1_VTABLE_PLACEMENT __in_data
#else
#define PICO_CORE1_VTABLE_PLACEMENT __in_bss
#endif
#endif

#ifndef __ASSEMBLER__

// PICO_CONFIG: PICO_NO_FPGA_CHECK, Remove the FPGA platform check for small code size reduction, type=bool, default=1, advanced=true, group=pico_runtime
#ifndef PICO_NO_FPGA_CHECK
#define PICO_NO_FPGA_CHECK 1
#endif

// PICO_CONFIG: PICO_NO_SIM_CHECK, Remove the SIM platform check for small code size reduction, type=bool, default=1, advanced=true, group=pico_runtime
#ifndef PICO_NO_SIM_CHECK
#define PICO_NO_SIM_CHECK 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PICO_NO_FPGA_CHECK
static inline bool running_on_fpga(void) {return false;}
#else
bool running_on_fpga(void);
#endif

#if PICO_NO_SIM_CHECK
static inline bool running_in_sim(void) {return false;}
#else
bool running_in_sim(void);
#endif

/*! \brief No-op function for the body of tight loops
 *  \ingroup pico_platform
 *
 * No-op function intended to be called by any tight hardware polling loop. Using this ubiquitously
 * makes it much easier to find tight loops, but also in the future \#ifdef-ed support for lockup
 * debugging might be added
 */
static __force_inline void tight_loop_contents(void) {}

#define host_safe_hw_ptr(x) ((uintptr_t)(x))
#define native_safe_hw_ptr(x) host_safe_hw_ptr(x)

#ifdef __cplusplus
}
#endif
#endif // __ASSEMBLER__


#endif