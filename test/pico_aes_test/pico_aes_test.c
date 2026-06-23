/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
// Include sys/types.h before inttypes.h to work around issue with
// certain versions of GCC and newlib which causes omission of PRIu64
#include <sys/types.h>
#include <inttypes.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/aes.h"
#include "pico/rand.h"
#include "hardware/rcp.h"

#define BUFFER_SIZE 10000
#define BUFFER_WORD_SIZE (BUFFER_SIZE/sizeof(uint32_t))


void pico_aes_lock_key(void) {
#if RC_COUNT
    rcp_count_check_nodelay(31 + PICO_AES256_RCP_COUNT_DELTA);
#endif
}
void pico_aes_lock_all(void) {}

int main() {
    stdio_init_all();
    printf("AES Test Starting\n");

    bool passing = true;
    for (int n=0; n < 10; n++) {
        uint32_t *data = malloc(BUFFER_SIZE);
        uint32_t *data_orig = malloc(BUFFER_SIZE);
        hard_assert(data);
        for (int i=0; i < BUFFER_WORD_SIZE; i++) data[i] = get_rand_32();
        printf("Buffer start   %08x %08x %08x\n", data[0], data[1], data[2]);
        memcpy(data_orig, data, BUFFER_SIZE);

        uint32_t* iv_public = malloc(16);
        hard_assert(iv_public);
        for (int i=0; i < 16/sizeof(uint32_t); i++) iv_public[i] = get_rand_32();

        pico_aes_try_decrypt((void*)data, BUFFER_SIZE, 29, (uint8_t*)iv_public);

        printf("Buffer encrypt %08x %08x %08x\n", data[0], data[1], data[2]);

        pico_aes_try_decrypt((void*)data, BUFFER_SIZE, 29, (uint8_t*)iv_public);

        printf("Buffer decrypt %08x %08x %08x\n", data[0], data[1], data[2]);
        printf("Buffer orig    %08x %08x %08x\n", data_orig[0], data_orig[1], data_orig[2]);

        for (int i=0; i < BUFFER_WORD_SIZE; i++) {
            if (data[i] != data_orig[i]) {
                printf("ERROR: Different values at %d: %08x vs %08x\n", i, data[i], data_orig[i]);
                passing = false;
                break;
            }
        }

        if (!passing) break;
        else printf("run %d passed\n", n);
    }

    if (passing) printf("PASSED\n");
    else printf("FAILED\n");
}
