/*
 * Copyright (c) 2026 Darrian
 * Copyright (c) 2024 Scott Shawcroft for Adafruit Industries
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/adafruit_metro_rp2350.h"

#ifndef _BOARDS_ADAFRUIT_METRO_RP2350_H
#define _BOARDS_ADAFRUIT_METRO_RP2350_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

// On some samples, the xosc can take longer to stabilize than is usual
#ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64
#endif

// For board detection
#define ADAFRUIT_METRO_RP2350

// --- RP2350 VARIANT ---
#define PICO_RP2350A 0

// --- BOARD SPECIFIC ---
#define ADAFRUIT_METRO_A0_PIN 41
#define ADAFRUIT_METRO_A1_PIN 42
#define ADAFRUIT_METRO_A2_PIN 43
#define ADAFRUIT_METRO_A3_PIN 44
#define ADAFRUIT_METRO_A4_PIN 45
#define ADAFRUIT_METRO_A5_PIN 46

// HSTX header
#define ADAFRUIT_METRO_DVI_CKN_PIN 15
#define ADAFRUIT_METRO_DVI_CKP_PIN 14
#define ADAFRUIT_METRO_DVI_D0N_PIN 19
#define ADAFRUIT_METRO_DVI_D0P_PIN 18
#define ADAFRUIT_METRO_DVI_D1N_PIN 17
#define ADAFRUIT_METRO_DVI_D1P_PIN 16
#define ADAFRUIT_METRO_DVI_D2N_PIN 13
#define ADAFRUIT_METRO_DVI_D2P_PIN 12

// SD and SDIO
#define ADAFRUIT_METRO_SD_SCK_PIN 34
#define ADAFRUIT_METRO_SDIO_CLOCK_PIN 34

#define ADAFRUIT_METRO_SD_MOSI_PIN 35
#define ADAFRUIT_METRO_SDIO_COMMAND_PIN 35

#define ADAFRUIT_METRO_SD_MISO_PIN 36
#define ADAFRUIT_METRO_SDIO_DATA0_PIN 36

#define ADAFRUIT_METRO_SDIO_DATA1_PIN 37
#define ADAFRUIT_METRO_SDIO_DATA2_PIN 38

#define ADAFRUIT_METRO_SD_CS_PIN 39
#define ADAFRUIT_METRO_SDIO_DATA3_PIN 39

#define ADAFRUIT_METRO_SD_CARD_DETECT_PIN 40

// USB host
#define ADAFRUIT_METRO_USB_HOST_DATA_PLUS_PIN 32
#define ADAFRUIT_METRO_USB_HOST_DATA_MINUS_PIN 33
#define ADAFRUIT_METRO_USB_HOST_5V_POWER_PIN 29

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
#define PICO_DEFAULT_LED_PIN 23
#endif

// --- RGB (NeoPixel) LED ---
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 25
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
// SPI 6 pin connector adjacent to the SD card
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 30
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 31
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 28
#endif

//------------- SD ------------
#ifndef PICO_SD_CARD_DETECT_PIN
#define PICO_SD_CARD_DETECT_PIN ADAFRUIT_METRO_SD_CARD_DETECT_PIN
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN ADAFRUIT_METRO_SDIO_CLOCK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN ADAFRUIT_METRO_SDIO_COMMAND_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN ADAFRUIT_METRO_SDIO_DATA0_PIN
#endif

#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif

#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 4
#endif

// --- PIO USB ---
#ifndef PICO_DEFAULT_PIO_USB_DP_PIN
#define PICO_DEFAULT_PIO_USB_DP_PIN ADAFRUIT_METRO_USB_HOST_DATA_PLUS_PIN
#endif

// --- FLASH ---
// Winbond W25Q128 (16MB) flash
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
#define PICO_PSRAM_CS_PIN 47
#endif

// PSRAM not fitted by default, so auto-detect
#ifndef PICO_AUTO_DETECT_PSRAM_SIZE
#define PICO_AUTO_DETECT_PSRAM_SIZE 1
#endif

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
