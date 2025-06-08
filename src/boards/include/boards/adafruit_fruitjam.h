/*
 * Copyright (c) 2024 FRUITJAM Board
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/fruitjam.h"

// pico_cmake_set PICO_PLATFORM=rp2350

#ifndef _BOARDS_FRUITJAM_H
#define _BOARDS_FRUITJAM_H

// For board detection
#define FRUITJAM_BOARD

// --- BOARD SPECIFIC ---
#define FRUITJAM_LED_PIN 29
#define FRUITJAM_NEOPIXEL_PIN 32
#define FRUITJAM_NEOPIXEL_COUNT 5
#define FRUITJAM_BUTTON1_PIN 0
#define FRUITJAM_BUTTON2_PIN 4
#define FRUITJAM_BUTTON3_PIN 5
#define FRUITJAM_PSRAM_CS_PIN 47

// SD Card pins
#define FRUITJAM_SD_DETECT_PIN 33
#define FRUITJAM_SD_CLK_PIN 34
#define FRUITJAM_SD_CMD_PIN 35
#define FRUITJAM_SD_DAT0_PIN 36
#define FRUITJAM_SD_DAT1_PIN 37
#define FRUITJAM_SD_DAT2_PIN 38
#define FRUITJAM_SD_CS_PIN 39

// I2S pins
#define FRUITJAM_I2S_DATA_PIN 24
#define FRUITJAM_I2S_WS_PIN 25
#define FRUITJAM_I2S_BCK_PIN 26
#define FRUITJAM_I2S_MCK_PIN 27
#define FRUITJAM_I2S_IRQ_PIN 23

// WiFi/ESP32 pins
#define FRUITJAM_WIFI_CS_PIN 46
#define FRUITJAM_WIFI_ACK_PIN 3
#define FRUITJAM_WIFI_RESET_PIN 22

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 8
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 9
#endif

// --- LED ---
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN FRUITJAM_LED_PIN
#endif

// --- NEOPIXEL/WS2812 ---
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN FRUITJAM_NEOPIXEL_PIN
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 20
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 21
#endif

// --- SPI ---
// Default SPI uses SD card interface
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN FRUITJAM_SD_CLK_PIN
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN FRUITJAM_SD_CMD_PIN
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN FRUITJAM_SD_DAT0_PIN
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN FRUITJAM_SD_CS_PIN
#endif

// --- FLASH ---
// Winbond W25Q128 (16MB) flash
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (16 * 1024 * 1024)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// --- RP2350 specific settings ---
// pico_cmake_set_default PICO_RP2350_A2_SUPPORTED = 1
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
