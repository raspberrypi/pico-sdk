/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_tinyfx.h"

#ifndef _BOARDS_PIMORONI_TINYFX_H
#define _BOARDS_PIMORONI_TINYFX_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define PIMORONI_TINYFX

// --- BOARD SPECIFIC ---
#define TINYFX_OUT1_PIN 3
#define TINYFX_OUT2_PIN 2
#define TINYFX_OUT3_PIN 4
#define TINYFX_OUT4_PIN 5
#define TINYFX_OUT5_PIN 8
#define TINYFX_OUT6_PIN 9

#define TINYFX_OUTR_PIN 13
#define TINYFX_OUTG_PIN 14
#define TINYFX_OUTB_PIN 15

#define TINYFX_I2C 0
#define TINYFX_SDA_PIN 16
#define TINYFX_SCL_PIN 17

#define TINYFX_I2S_DATA_PIN 18
#define TINYFX_I2S_BCLK_PIN 19
#define TINYFX_I2S_LRCLK_PIN 20
#define TINYFX_AMP_EN_PIN 21

#define TINYFX_USER_SW_PIN 22

#define TINYFX_SENSOR_PIN 26
#define TINYFX_VSENSE_PIN 28

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- LED ---
// no PICO_DEFAULT_LED_PIN
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
// routed to Qw/St connector
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C TINYFX_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN TINYFX_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN TINYFX_SCL_PIN
#endif

// --- SPI ---
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

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (4 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 0
#endif

// --- CYW43 ---
// only fitted on Tiny FX W

// cyw43 SPI pins can't be changed at runtime
#ifndef CYW43_PIN_WL_DYNAMIC
#define CYW43_PIN_WL_DYNAMIC 0
#endif

// gpio pin to power up the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 23u
#endif

// gpio pin for spi data out to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 24u
#endif

// gpio pin for spi data in from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 24u
#endif

// gpio (irq) pin for the irq line from the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 24u
#endif

// gpio pin for the spi clock line to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 29u
#endif

// gpio pin for the spi chip select to the cyw43 chip
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 25u
#endif

#endif
