/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------
//
//------------------------------------------------------------------------------------------
// Board definition for the E Electronic Parts "Pico_Mini RP2040"
//
// This header may be included by other board headers as "boards/eelectronicparts_picomini.h"

// pico_cmake_set PICO_PLATFORM=rp2040

#ifndef _BOARDS_EELECTRONICPARTS_PICOMINI_H
#define _BOARDS_EELECTRONICPARTS_PICOMINI_H

// For board detection
#define EELECTRONICPARTS_PICOMINI

// // copied from itsybitsy rp2040 idk why
// #ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
// #define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64
// #endif

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN   25
#endif
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 16
#endif

#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C       0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN   16
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN   17
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 10
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 11
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 12
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 13
#endif

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (8 * 1024 * 1024)
#ifndef PICO_FLASH_SIZE_BYTES
// This board comes in several variants with different flash sizes
// Default to 8MB, but there are 2MB, 4MB, and 16MB variants as well
#define PICO_FLASH_SIZE_BYTES (8 * 1024 * 1024)
#endif
// All boards have B1 RP2040

#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 0
#endif

#endif
