/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_RASPBERRY_PI_PI500_H
#define _BOARDS_RASPBERRY_PI_PI500_H

#include "boards/pico2.h"

// For board detection
#define RASPBERRY_PI_PI500

// --- FLASH CONFIGURATION ---
// Pi 500 uses W25X10CL flash in DSPI mode rather than QSPI
#undef PICO_BOOT_STAGE2_CHOOSE_W25Q080
#define PICO_BOOT_STAGE2_CHOOSE_W25X10CL 1

// --- USB CONFIGURATION ---
// Crucial for Pi 500 keyboard function when Pi is off - ignores USB startup check
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 0
#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED 0

// --- BOOTLOADER CONFIGURATION ---
// Disable double tap reset timeout for Pi 500
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED 0

// --- KEYBOARD MATRIX CONFIGURATION ---
// Pi 500 keyboard matrix pins (for reference)
// Rows: GP0-GP7 (8 rows)
// Cols: GP27,GP8-GP15,GP18,GP20-GP24,GP26-GP29 (18 cols)
// Matrix scanning: ROW2COL, no diodes (ghost keys possible)

// --- DEBUG UART ---
// Spare pin for debug UART TX
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 16
#endif

// --- PI 500 SPECIFIC PINS ---
// These pins are used by Pi 500 hardware and should be avoided in user applications
// GP0-GP7: Keyboard matrix rows
// GP8-GP15: Keyboard matrix columns (partial)
// GP16: Debug UART TX
// GP18,GP20-GP24: Keyboard matrix columns (partial)
// GP26-GP29: Keyboard matrix columns (partial)

#endif