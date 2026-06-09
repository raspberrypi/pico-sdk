/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/unique_id.h"

PICO_WEAK_FUNCTION_DEF(pico_get_unique_board_id)
void PICO_WEAK_FUNCTION_IMPL_NAME(pico_get_unique_board_id)(pico_unique_board_id_t *id_out) {
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
        id_out->id[i] = 0xa0 + i;
    }
}

PICO_WEAK_FUNCTION_DEF(pico_get_unique_board_id_string)
void PICO_WEAK_FUNCTION_IMPL_NAME(pico_get_unique_board_id_string)(char *id_out, uint len) {
    assert(len > 0);
    size_t i;

    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);

    // Generate hex one nibble at a time
    for (i = 0; (i < len - 1) && (i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2); i++) {
        int nibble = (id.id[i/2] >> (4 - 4 * (i&1))) & 0xf;
        id_out[i] = (char)(nibble < 10 ? nibble + '0' : nibble + 'A' - 10);
    }
    id_out[i] = 0;
}
