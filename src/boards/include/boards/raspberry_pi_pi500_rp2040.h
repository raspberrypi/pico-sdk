/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_RASPBERRY_PI_PI500_RP2040_H
#define _BOARDS_RASPBERRY_PI_PI500_RP2040_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define RASPBERRY_PI_PI500_RP2040

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 16  // Pi 500 debug UART on GP16
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1   // Standard RX pin
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 17  // Pi 500 LED on GP17
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 4
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 5
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 18
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 19
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 16
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 17
#endif

// --- FLASH ---
// Pi 500 uses W25X10CL flash (differs from standard Pico W25Q080)
#define PICO_BOOT_STAGE2_CHOOSE_W25X10CL 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (1 * 1024 * 1024))  // 1MB W25X10CL
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
#endif

// Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
#define PICO_SMPS_MODE_PIN 23

#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1
#endif

// The GPIO Pin used to read VBUS to determine if the device is battery powered.
#ifndef PICO_VBUS_PIN
#define PICO_VBUS_PIN 24
#endif

// The GPIO Pin used to monitor VSYS. Typically you would use this with ADC.
#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN 29
#endif

// --- USB CONFIGURATION ---
// Crucial for Pi 500 keyboard function when Pi is off - ignores USB startup check
#define PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE 0
#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED 0

// --- BOOTLOADER CONFIGURATION ---
// Disable double tap reset timeout for Pi 500
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED 0

// --- PI 500 KEYBOARD MATRIX CONFIGURATION ---
// Complete GPIO pin mapping for 8×18 keyboard matrix
// Rows (8): GP0, GP1, GP2, GP3, GP4, GP5, GP6, GP7
// Cols (18): GP27, GP8, GP9, GP10, GP11, GP12, GP13, GP14, GP23, GP24, GP22, GP20, GP29, GP18, GP15, GP21, GP28, GP26
// Matrix scanning: ROW2COL, no diodes (ghost keys possible with 3+ key combinations)

// --- PI 500 SYSTEM PINS ---
// These pins are reserved for Pi 500 hardware functions:
// GP0-GP7: Keyboard matrix rows
// GP8-GP15: Keyboard matrix columns (partial)
// GP16: Debug UART TX
// GP17: LED (heartbeat/debug)
// GP18: Keyboard matrix column
// GP19: Power button control (PWR_BTN) - CRITICAL for Pi power management
// GP20: Power key detection column (separate from matrix)
// GP21-GP24: Keyboard matrix columns
// GP25: Caps Lock LED
// GP26-GP29: Keyboard matrix columns

// --- POWER MANAGEMENT WARNING ---
// Custom firmware MUST implement power button handling via GP19 or the Pi 500
// power button will stop working, potentially making the device unbootable.
// See QMK pi500.c for reference implementation.

#endif