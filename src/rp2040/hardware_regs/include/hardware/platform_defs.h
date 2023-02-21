/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_PLATFORM_DEFS_H
#define _HARDWARE_PLATFORM_DEFS_H

// This header is included from C and assembler - intended mostly for #defines; guard other stuff with #ifdef __ASSEMBLER__

#ifndef _u
#ifdef __ASSEMBLER__
#define _u(x) x
#else
#define _u(x) x ## u
#endif
#endif

#define NUM_CORES _u(2)
#define NUM_DMA_CHANNELS _u(12)
#define NUM_DMA_TIMERS _u(4)
#define NUM_IRQS _u(32)
#define NUM_USER_IRQS _u(6)
#define NUM_PIOS _u(2)
#define NUM_PIO_STATE_MACHINES _u(4)
#define NUM_PWM_SLICES _u(8)
#define NUM_SPIN_LOCKS _u(32)
#define NUM_UARTS _u(2)
#define NUM_I2CS _u(2)
#define NUM_SPIS _u(2)
#define NUM_TIMERS _u(4)
#define NUM_ADC_CHANNELS _u(5)

#define NUM_BANK0_GPIOS _u(30)
#define NUM_QSPI_GPIOS _u(6)

#define PIO_INSTRUCTION_COUNT _u(32)

// For USB operation this *has* to be 48 MHz
#define USB_CLK_MHZ _u(48)

// PICO_CONFIG: SYS_CLK_MHZ, The system operating frequency in MHz, type=int, default=125, advanced=true, group=hardware_base
#ifndef SYS_CLK_MHZ
#define SYS_CLK_MHZ _u(125)
#endif

// PICO_CONFIG: XOSC_MHZ, The crystal oscillator frequency in MHz, type=int, default=12, advanced=true, group=hardware_base
#ifndef XOSC_MHZ
#define XOSC_MHZ _u(12)
#endif

 // Check for standard set-up
#if (XOSC_MHZ == 12 && SYS_CLK_MHZ == 125)
/// \tag::pll_settings[]
// Configure PLLs
//                   REF     FBDIV VCO            POSTDIV
// PLL SYS: 12 / 1 = 12MHz * 125 = 1500MHz / 6 / 2 = 125MHz
// PLL USB: 12 / 1 = 12MHz * 100 = 1200MHz / 5 / 5 =  48MHz
/// \end::pll_settings[]
#define PLL_SYS_VCO_FREQ_MHZ                1500
#define PLL_SYS_POSTDIV1                    6
#define PLL_SYS_POSTDIV2                    2

#define PLL_USB_VCO_FREQ_MHZ                1200
#define PLL_USB_POSTDIV1                    5
#define PLL_USB_POSTDIV2                    5

// Note: Do not trivially change this
#define PLL_COMMON_REFDIV                   1
#else
#error Use vcocalc.py to calculate correct values for the revised XOSC and/or system clock frequencies and define here.
#endif

#define FIRST_USER_IRQ (NUM_IRQS - NUM_USER_IRQS)
#define VTABLE_FIRST_IRQ 16

#endif

