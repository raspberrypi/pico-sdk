/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/flash.h"
#include "pico/unique_id.h"

static_assert(PICO_UNIQUE_BOARD_ID_SIZE_BYTES == FLASH_UNIQUE_ID_SIZE_BYTES, "Board ID size must match flash ID size");

static pico_unique_board_id_t retrieved_id;

static void __attribute__((constructor)) _retrieve_unique_id_on_boot(void) {
#if PICO_NO_FLASH
    // The hardware_flash call will panic() if called directly on a NO_FLASH
    // build. Since this constructor is pre-main it would be annoying to
    // debug, so just produce something well-defined and obviously wrong.
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
        retrieved_id.id[i] = 0xee;
#else
    flash_get_unique_id(retrieved_id.id);
#endif
}

void pico_get_unique_board_id(pico_unique_board_id_t *id_out) {
    *id_out = retrieved_id;
}
