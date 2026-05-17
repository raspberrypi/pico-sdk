/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/debug_probe.h"

#ifndef _BOARDS_DEBUG_PROBE_H
#define _BOARDS_DEBUG_PROBE_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define RASPBERRYPI_DEBUG_PROBE

// --- BOARD SPECIFIC ---
#define DEBUG_PROBE_USB_CONNECTED_LED_PIN 2
#define DEBUG_PROBE_DAP_CONNECTED_LED_PIN 15
#define DEBUG_PROBE_DAP_RUNNING_LED_PIN 16
#define DEBUG_PROBE_UART_RX_LED_PIN 7
#define DEBUG_PROBE_UART_TX_LED_PIN 8

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 1
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 4
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 5
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN DEBUG_PROBE_USB_CONNECTED_LED_PIN
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1
#endif

#endif
