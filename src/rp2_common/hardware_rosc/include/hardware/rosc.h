/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_ROSC_H_
#define _HARDWARE_ROSC_H_

#include "pico.h"
#include "hardware/structs/rosc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file rosc.h
 *  \defgroup hardware_rosc hardware_rosc
 *
 * Ring Oscillator (ROSC) API
 *
 * A Ring Oscillator is an on-chip oscillator that requires no external crystal. Instead, the output is generated from a series of
 * inverters that are chained together to create a feedback loop. RP2 chips boot from the ring oscillator initially, meaning the
 * first stages of the bootrom, including booting from SPI flash, will be clocked by the ring oscillator. If your design has a
 * crystal oscillator, you’ll likely want to switch to this as your reference clock as soon as possible, because the frequency is
 * more accurate than the ring oscillator.
 */

/*! \brief  Disable the Ring Oscillator
 *  \ingroup hardware_rosc
 *
 */
void rosc_disable(void);

/*! \brief  Put Ring Oscillator in to dormant mode.
 *  \ingroup hardware_rosc
 *
 * The ROSC supports a dormant mode, which stops oscillation until woken up up by an asynchronous interrupt.
 * This can either come from the RTC, being clocked by an external clock, or a GPIO pin going high or low.
 * If no IRQ is configured before going into dormant mode the ROSC will never restart.
 *
 * PLLs should be stopped before selecting dormant mode.
 */
void rosc_set_dormant(void);

/*! \brief Re-enable the ring oscillator so that the processor cores can wake up after sleep/dormant mode
    \ingroup hardware_rosc

    This must be called at the end of the sleeping period (e.g., in an interrupt service routine)
*/
void rosc_restart(void);

/*! \brief  Measure the frequency of the Ring Oscillator
 *  \ingroup hardware_rosc
 *
 * This will only be accurate if clk_ref is currently running from an accurate source (eg the XOSC).
 *
 * \return The frequency of the Ring Oscillator in kHz.
 */
uint rosc_measure_freq_khz(void);

// ROSC BADWRITE is not reliable, see RP2040-E10
// inline static void rosc_clear_bad_write(void) {
//     hw_clear_bits(&rosc_hw->status, ROSC_STATUS_BADWRITE_BITS);
// }

// inline static bool rosc_write_okay(void) {
//     return !(rosc_hw->status & ROSC_STATUS_BADWRITE_BITS);
// }

/*! \brief  Checked write to a Ring Oscillator register
 *  \ingroup hardware_rosc
 * 
 * This would normally clear the bad write flag and asserts that the write is okay.
 * However, ROSC BADWRITE is not reliable (see RP2040-E10) so we don't do this.
 *
 * \param addr The register address.
 * \param value The value to write.
 */
inline static void rosc_write(io_rw_32 *addr, uint32_t value) {
    // rosc_clear_bad_write();
    // assert(rosc_write_okay());
    *addr = value;
    // assert(rosc_write_okay());
};

#ifdef __cplusplus
}
#endif

#endif