/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/pimoroni_presto.h"

#ifndef _BOARDS_PIMORONI_PRESTO_H
#define _BOARDS_PIMORONI_PRESTO_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define PIMORONI_PRESTO

// --- BOARD SPECIFIC ---
// 480x480 display on an 18 bit parallel interface, data on GPIO 1 through 18
#define PRESTO_LCD_D0_PIN 1
#define PRESTO_LCD_D1_PIN 2
#define PRESTO_LCD_D2_PIN 3
#define PRESTO_LCD_D3_PIN 4
#define PRESTO_LCD_D4_PIN 5
#define PRESTO_LCD_D5_PIN 6
#define PRESTO_LCD_D6_PIN 7
#define PRESTO_LCD_D7_PIN 8
#define PRESTO_LCD_D8_PIN 9
#define PRESTO_LCD_D9_PIN 10
#define PRESTO_LCD_D10_PIN 11
#define PRESTO_LCD_D11_PIN 12
#define PRESTO_LCD_D12_PIN 13
#define PRESTO_LCD_D13_PIN 14
#define PRESTO_LCD_D14_PIN 15
#define PRESTO_LCD_D15_PIN 16
#define PRESTO_LCD_D16_PIN 17
#define PRESTO_LCD_D17_PIN 18
#define PRESTO_LCD_HSYNC_PIN 19
#define PRESTO_LCD_VSYNC_PIN 20
#define PRESTO_LCD_DE_PIN 21
#define PRESTO_LCD_DOT_CLK_PIN 22

// display configuration interface
#define PRESTO_LCD_SPI 1
#define PRESTO_LCD_CLK_PIN 26
#define PRESTO_LCD_DAT_PIN 27
#define PRESTO_LCD_CS_PIN 28

#define PRESTO_TOUCH_I2C 1
#define PRESTO_TOUCH_SDA_PIN 30
#define PRESTO_TOUCH_SCL_PIN 31
#define PRESTO_TOUCH_INT_PIN 32

#define PRESTO_LED_DAT_PIN 33

#define PRESTO_SD_SPI 0
#define PRESTO_SD_SCK_PIN 34
#define PRESTO_SD_MOSI_PIN 35
#define PRESTO_SD_MISO_PIN 36
#define PRESTO_SD_CS_PIN 39

#define PRESTO_I2C 0
#define PRESTO_SDA_PIN 40
#define PRESTO_SCL_PIN 41

#define PRESTO_BUZZER_PIN 43
#define PRESTO_BACKLIGHT_PIN 45
#define PRESTO_PSRAM_CS_PIN 47

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0

// --- UART ---
// no UART pins are broken out
// no PICO_DEFAULT_UART
// no PICO_DEFAULT_UART_TX_PIN
// no PICO_DEFAULT_UART_RX_PIN

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// ambient LEDs behind the display
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN PRESTO_LED_DAT_PIN
#endif

// --- I2C ---
// routed to Qw/St connector
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C PRESTO_I2C
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN PRESTO_SDA_PIN
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN PRESTO_SCL_PIN
#endif

// --- SPI ---
// routed to the microSD slot
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI PRESTO_SD_SPI
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN PRESTO_SD_SCK_PIN
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN PRESTO_SD_MOSI_PIN
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN PRESTO_SD_MISO_PIN
#endif
// PRESTO_SD_CS_PIN is GPIO 39 (not an SPI0 CSn pin) and must be driven manually
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

// --- PSRAM ---
#ifndef PICO_PSRAM_CS_PIN
#define PICO_PSRAM_CS_PIN PRESTO_PSRAM_CS_PIN
#endif

pico_board_cmake_set_default(PICO_PSRAM_SIZE_BYTES, (8 * 1024 * 1024))
#ifndef PICO_PSRAM_SIZE_BYTES
#define PICO_PSRAM_SIZE_BYTES (8 * 1024 * 1024)
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// no PICO_SMPS_MODE_PIN
// no PICO_VBUS_PIN
// no PICO_VSYS_PIN

// --- CYW43 ---

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
