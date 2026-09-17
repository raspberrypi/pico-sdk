/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_SOLDERED_NULA_ETHERNET_W55RP20_H
#define _BOARDS_SOLDERED_NULA_ETHERNET_W55RP20_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define SOLDERED_NULA_ETHERNET_W55RP20

// --- BOARD SPECIFIC ---
#ifndef SOLDERED_NULA_ETHERNET_SD_ENABLE
#define SOLDERED_NULA_ETHERNET_SD_ENABLE 9
#endif

#ifndef SOLDERED_NULA_ETHERNET_USER_BTN 
#define SOLDERED_NULA_ETHERNET_USER_BTN 8
#endif

// --- LED ---
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 1
#endif

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

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 1
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 2
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 3
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 0
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 6
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 7
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 4
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 5
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

// Soldered NULA Ethernet has an SD Card.
#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN 10
#endif
#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN 12
#endif
#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN 11
#endif
#ifndef PICO_SD_DAT3_PIN
#define PICO_SD_DAT3_PIN 13 // DAT3 of the SD card is the chip select pin
#endif
#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 1
#endif

// The following pins are used internally between 
// the RP2040 and W5500 chip and should NOT be used for other purposes
#ifndef W5500_DEFAULT_PIN_ETH_SPI_CS
#define W5500_DEFAULT_PIN_ETH_SPI_CS 20
#endif
#ifndef W5500_DEFAULT_PIN_ETH_SPI_SCK
#define W5500_DEFAULT_PIN_ETH_SPI_SCK 21
#endif
#ifndef W5500_DEFAULT_PIN_ETH_SPI_MISO
#define W5500_DEFAULT_PIN_ETH_SPI_MISO 22
#endif
#ifndef W5500_DEFAULT_PIN_ETH_SPI_MOSI
#define W5500_DEFAULT_PIN_ETH_SPI_MOSI 23
#endif
#ifndef W5500_DEFAULT_PIN_ETH_INT
#define W5500_DEFAULT_PIN_ETH_INT 24
#endif
#ifndef W5500_DEFAULT_PIN_ETH_RST
#define W5500_DEFAULT_PIN_ETH_RST 25
#endif

#endif
