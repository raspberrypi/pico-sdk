/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/flash.h"
#include "pico/unique_id.h"

static_assert(PICO_UNIQUE_ID_SIZE_BYTES == FLASH_UNIQUE_ID_SIZE_BYTES);

static uint8_t retrieved_id[PICO_UNIQUE_ID_SIZE_BYTES];

static void __attribute__((constructor)) _retrieve_unique_id_on_boot() {
#if PICO_NO_FLASH
    // The hardware_flash call will panic() if called directly on a NO_FLASH
    // build. Since this constructor is pre-main it would be annoying to
    // debug, so just produce something well-defined and obviously wrong.
    for (int i = 0; i < PICO_UNIQUE_ID_SIZE_BYTES; i++)
        retrieved_id[i] = 0xee;
#else
    flash_get_unique_id(retrieved_id);
#endif
}

void pico_get_unique_id(uint8_t *id_out) {
    for (int i = 0; i < PICO_UNIQUE_ID_SIZE_BYTES; i++)
        id_out[i] = retrieved_id[i];
}
