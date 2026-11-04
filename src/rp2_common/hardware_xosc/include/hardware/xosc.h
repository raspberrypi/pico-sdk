/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_XOSC_H
#define _HARDWARE_XOSC_H

#include "pico.h"
#include "hardware/structs/xosc.h"

// For frequency related definitions etc
#include "hardware/clocks.h"


// Allow lengthening startup delay to accommodate slow-starting oscillators

// PICO_CONFIG: PICO_XOSC_STARTUP_DELAY_MULTIPLIER, Multiplier (from 1ms) for xosc startup delay to accommodate slow-starting oscillators, type=int, min=1, default=6, group=hardware_xosc
#ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 6
#endif

// PICO_CONFIG: PICO_XOSC_FREQ_RANGE_MAX, The maximum frequency in MHz for the XOSC to support (not required when using CMOS clock input instead of XOSC) - defining this variable is not supported on RP2040 as it only has one range, type=int, min=1, max=100, default=XOSC_HZ/MHZ, group=hardware_xosc
#if PICO_RP2040
#define PICO_XOSC_FREQ_RANGE_MAX 15
#else
#ifndef PICO_XOSC_FREQ_RANGE_MAX
#define PICO_XOSC_FREQ_RANGE_MAX (XOSC_HZ / MHZ)
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

/** \file hardware/xosc.h
 *  \defgroup hardware_xosc hardware_xosc
 *
 * \brief Crystal Oscillator (XOSC) API
 */

/*! \brief  Initialise the crystal oscillator system
 *  \ingroup hardware_xosc
 *
 * This function will block until the crystal oscillator has stabilised.
 **/
void xosc_init(void);

/*! \brief  Disable the Crystal oscillator
 *  \ingroup hardware_xosc
 *
 * Turns off the crystal oscillator source, and waits for it to become unstable
 **/
void xosc_disable(void);

/*! \brief Set the crystal oscillator system to dormant
 *  \ingroup hardware_xosc
 *
 * Turns off the crystal oscillator until it is woken by an interrupt. This will block and hence
 * the entire system will stop, until an interrupt wakes it up. This function will
 * continue to block until the oscillator becomes stable after its wakeup.
 **/
void xosc_dormant(void);

#ifdef __cplusplus
}
#endif

#endif
