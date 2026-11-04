/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_yukon.h"

#ifndef _BOARDS_PIMORONI_YUKON_H
#define _BOARDS_PIMORONI_YUKON_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define PIMORONI_YUKON

// --- BOARD SPECIFIC ---
// six module slots, each with four fast IO wired directly to the RP2040
// (each slot also has three slow IO on the I2C IO expanders)
#define YUKON_NUM_SLOTS 6

#define YUKON_SLOT1_FAST1_PIN 0
#define YUKON_SLOT1_FAST2_PIN 1
#define YUKON_SLOT1_FAST3_PIN 2
#define YUKON_SLOT1_FAST4_PIN 3

#define YUKON_SLOT2_FAST1_PIN 4
#define YUKON_SLOT2_FAST2_PIN 5
#define YUKON_SLOT2_FAST3_PIN 6
#define YUKON_SLOT2_FAST4_PIN 7

#define YUKON_SLOT3_FAST1_PIN 8
#define YUKON_SLOT3_FAST2_PIN 9
#define YUKON_SLOT3_FAST3_PIN 10
#define YUKON_SLOT3_FAST4_PIN 11

#define YUKON_SLOT4_FAST1_PIN 12
#define YUKON_SLOT4_FAST2_PIN 13
#define YUKON_SLOT4_FAST3_PIN 14
#define YUKON_SLOT4_FAST4_PIN 15

#define YUKON_SLOT5_FAST1_PIN 16
#define YUKON_SLOT5_FAST2_PIN 17
#define YUKON_SLOT5_FAST3_PIN 18
#define YUKON_SLOT5_FAST4_PIN 19

#define YUKON_SLOT6_FAST1_PIN 20
#define YUKON_SLOT6_FAST2_PIN 21
#define YUKON_SLOT6_FAST3_PIN 22
#define YUKON_SLOT6_FAST4_PIN 23

#define YUKON_I2C 0
#define YUKON_SDA_PIN 24
#define YUKON_SCL_PIN 25
#define YUKON_INT_PIN 28

// expansion pins, also usable as SPI1 SCK and TX, or as I2C1
#define YUKON_A0_PIN 26
#define YUKON_A1_PIN 27
#define YUKON_NUM_ADC_PINS 2

#define YUKON_SHARED_ADC_PIN 29

// --- UART ---
// no PICO_DEFAULT_UART
// no PICO_DEFAULT_UART_TX_PIN
// no PICO_DEFAULT_UART_RX_PIN

// --- LED ---
// no PICO_DEFAULT_LED_PIN
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
// serves the module slots via the IO expanders, and the Qw/ST connectors
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C YUKON_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN YUKON_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN YUKON_SCL_PIN
#endif

// --- SPI ---
// SPI1 SCK and TX are on the expansion pins, there is no MISO or CSn
// no PICO_DEFAULT_SPI
// no PICO_DEFAULT_SPI_SCK_PIN
// no PICO_DEFAULT_SPI_TX_PIN
// no PICO_DEFAULT_SPI_RX_PIN
// no PICO_DEFAULT_SPI_CSN_PIN

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 0
#endif

#endif
