/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_BTSTACK_FLASH_BANK_H
#define _PICO_BTSTACK_FLASH_BANK_H

#include "pico.h"
#include "hardware/flash.h"
#include "hal_flash_bank.h"

#ifdef __cplusplus
extern "C" {
#endif

// PICO_CONFIG: PICO_FLASH_BANK_TOTAL_SIZE, Total size of the Bluetooth flash storage. Must be an even multiple of FLASH_SECTOR_SIZE, type=int, default=FLASH_SECTOR_SIZE * 2, group=pico_btstack
#ifndef PICO_FLASH_BANK_TOTAL_SIZE
#define PICO_FLASH_BANK_TOTAL_SIZE (FLASH_SECTOR_SIZE * 2u)
#endif

// PICO_CONFIG: PICO_FLASH_BANK_STORAGE_OFFSET, Offset in flash of the Bluetooth flash storage, type=int, default=PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE on RP2350 otherwise PICO_FLASH_SIZE_BYTES - PICO_FLASH_BANK_TOTAL_SIZE, group=pico_btstack
#ifndef PICO_FLASH_BANK_STORAGE_OFFSET
#if PICO_RP2350 && PICO_RP2350_A2_SUPPORTED
#define PICO_FLASH_BANK_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE - PICO_FLASH_BANK_TOTAL_SIZE)
#else
#define PICO_FLASH_BANK_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - PICO_FLASH_BANK_TOTAL_SIZE)
#endif
#endif

/**
 * \brief Return the singleton BTstack HAL flash instance, used for non-volatile storage
 * \ingroup pico_btstack
 *
 * \note By default, two sectors near the end of flash are used.
 * For RP2350 when PICO_RP2350_A2_SUPPORTED is true, two sectors that are three sectors from the end of flash are used.
 * This keeps the last sector free for a workaround for chip errata RP2350-E10. See the RP2350 datasheet for more details about this.
 * Otherwise, two sectors directly at the end of flash are used.
 * See \c PICO_FLASH_BANK_STORAGE_OFFSET and \c PICO_FLASH_BANK_TOTAL_SIZE)
 */
const hal_flash_bank_t *pico_flash_bank_instance(void);

#ifdef __cplusplus
}
#endif
#endif
