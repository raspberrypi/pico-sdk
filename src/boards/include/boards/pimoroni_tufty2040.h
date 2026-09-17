/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_tufty2040.h"

#ifndef _BOARDS_PIMORONI_TUFTY2040_H
#define _BOARDS_PIMORONI_TUFTY2040_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define PIMORONI_TUFTY2040

// --- BOARD SPECIFIC ---
#define TUFTY2040_SW_DOWN_PIN 6
#define TUFTY2040_SW_A_PIN 7
#define TUFTY2040_SW_B_PIN 8
#define TUFTY2040_SW_C_PIN 9
#define TUFTY2040_SW_UP_PIN 22
#define TUFTY2040_USER_SW_PIN 23

#define TUFTY2040_LED_PIN 25
#define TUFTY2040_BACKLIGHT_PIN 2

// routed to Qw/ST connector
#define TUFTY2040_I2C 0
#define TUFTY2040_INT_PIN 3
#define TUFTY2040_SDA_PIN 4
#define TUFTY2040_SCL_PIN 5

// 8 bit parallel display interface, data on GPIO 14 through 21
#define TUFTY2040_LCD_CS_PIN 10
#define TUFTY2040_LCD_DC_PIN 11
#define TUFTY2040_LCD_WR_PIN 12
#define TUFTY2040_LCD_RD_PIN 13
#define TUFTY2040_LCD_D0_PIN 14
#define TUFTY2040_LCD_D1_PIN 15
#define TUFTY2040_LCD_D2_PIN 16
#define TUFTY2040_LCD_D3_PIN 17
#define TUFTY2040_LCD_D4_PIN 18
#define TUFTY2040_LCD_D5_PIN 19
#define TUFTY2040_LCD_D6_PIN 20
#define TUFTY2040_LCD_D7_PIN 21

#define TUFTY2040_VBUS_DETECT_PIN 24
#define TUFTY2040_LIGHT_SENSE_PIN 26
// powers the light sensor
#define TUFTY2040_SENSOR_POWER_PIN 27
#define TUFTY2040_VREF_1V24_PIN 28
#define TUFTY2040_VBAT_SENSE_PIN 29

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
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN TUFTY2040_LED_PIN
#endif
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C TUFTY2040_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN TUFTY2040_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN TUFTY2040_SCL_PIN
#endif

// --- SPI ---
// GPIO 16 through 19 are the display data bus
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

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (8 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (8 * 1024 * 1024)
#endif

#ifndef PICO_VBUS_PIN
#define PICO_VBUS_PIN TUFTY2040_VBUS_DETECT_PIN
#endif

// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 0
#endif

#endif
