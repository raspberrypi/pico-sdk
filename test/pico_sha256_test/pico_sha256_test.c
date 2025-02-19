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
#include "pico/sha256.h"

#define BUFFER_SIZE 10000

static void run_test(bool use_dma) {
    pico_sha256_state_t state;

    // Test empty
    const uint8_t empty_expected[] = { \
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, \
        0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, \
        0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, \
        0xb8, 0x55 };

    sha256_result_t result;

    int rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, NULL, 0);
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(empty_expected, &result, SHA256_RESULT_BYTES) == 0);

    // nist 1
    const uint8_t nist_1[] = { 0x61, 0x62, 0x63 };
    const uint8_t nist_1_expected[] = { \
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, \
        0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, \
        0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, \
        0x15, 0xad };

    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, nist_1, sizeof(nist_1));
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(nist_1_expected, &result.bytes, SHA256_RESULT_BYTES) == 0);

    // RC4.16
    const uint8_t rc_4_16[] = { \
        0xde, 0x18, 0x89, 0x41, 0xa3, 0x37, 0x5d, 0x3a, 0x8a, 0x06, \
        0x1e, 0x67, 0x57, 0x6e, 0x92, 0x6d };
    const uint8_t rc_4_16_expected[] = { \
        0x06, 0x7c, 0x53, 0x12, 0x69, 0x73, 0x5c, 0xa7, 0xf5, 0x41, \
        0xfd, 0xac, 0xa8, 0xf0, 0xdc, 0x76, 0x30, 0x5d, 0x3c, 0xad, \
        0xa1, 0x40, 0xf8, 0x93, 0x72, 0xa4, 0x10, 0xfe, 0x5e, 0xff, \
        0x6e, 0x4d };

    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, rc_4_16, sizeof(rc_4_16));
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(rc_4_16_expected, &result.bytes, SHA256_RESULT_BYTES) == 0);

    // RC4.55
    const uint8_t rc_4_55[] = { \
        0xde, 0x18, 0x89, 0x41, 0xa3, 0x37, 0x5d, 0x3a, 0x8a, 0x06, \
        0x1e, 0x67, 0x57, 0x6e, 0x92, 0x6d, 0xc7, 0x1a, 0x7f, 0xa3, \
        0xf0, 0xcc, 0xeb, 0x97, 0x45, 0x2b, 0x4d, 0x32, 0x27, 0x96, \
        0x5f, 0x9e, 0xa8, 0xcc, 0x75, 0x07, 0x6d, 0x9f, 0xb9, 0xc5, \
        0x41, 0x7a, 0xa5, 0xcb, 0x30, 0xfc, 0x22, 0x19, 0x8b, 0x34, \
        0x98, 0x2d, 0xbb, 0x62, 0x9e };
    const uint8_t rc_4_55_expected[] = { \
        0x03, 0x80, 0x51, 0xe9, 0xc3, 0x24, 0x39, 0x3b, 0xd1, 0xca, \
        0x19, 0x78, 0xdd, 0x09, 0x52, 0xc2, 0xaa, 0x37, 0x42, 0xca, \
        0x4f, 0x1b, 0xd5, 0xcd, 0x46, 0x11, 0xce, 0xa8, 0x38, 0x92, \
        0xd3, 0x82 };

    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, rc_4_55, sizeof(rc_4_55));
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(rc_4_55_expected, result.bytes, SHA256_RESULT_BYTES) == 0);

    // nist 3
    uint8_t *buffer = malloc(10000);
    memset(buffer, 0x61, BUFFER_SIZE);
    const uint8_t nist_3_expected[] = { \
        0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, \
        0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67, 0xf1, 0x80, 0x9a, 0x48, \
        0xa4, 0x97, 0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, \
        0x2c, 0xd0 };

    uint64_t start = time_us_64();
    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    for(int i = 0; i < 1000000; i += BUFFER_SIZE) {
        pico_sha256_update_blocking(&state, buffer, BUFFER_SIZE);
    }
    pico_sha256_finish(&state, &result);
    uint64_t pico_time = time_us_64() - start;
    printf("Pico hw time for sha256 of 1M bytes %s DMA %"PRIu64"ms\n", use_dma ? "with" : "without", pico_time / 1000);
    hard_assert(memcmp(nist_3_expected, result.bytes, SHA256_RESULT_BYTES) == 0);

    // Cause an error
    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update(&state, buffer, BUFFER_SIZE); // non-blocking!
    if (use_dma) {
        assert(dma_channel_is_busy(state.channel));
        dma_channel_wait_for_finish_blocking(state.channel);
        dma_channel_configure(
            state.channel,
            &state.config,
            sha256_get_write_addr(),
            buffer,
            BUFFER_SIZE,
            true
        );
        dma_channel_wait_for_finish_blocking(state.channel);
    } else {
        // If we're not using DMA, write a word at a time
        for(int i = 0; i < BUFFER_SIZE; i += sizeof(uint32_t)) {
            sha256_put_word(*((uint32_t*)(buffer + i)));
        }
    }
    sha256_wait_ready_blocking();
    hard_assert(sha256_err_not_ready());
    pico_sha256_finish(&state, NULL); // passing null to just release the hardware

    // check we can restart
    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);

    // Check hardware is claimed
    pico_sha256_state_t duff = {0};
    rc = pico_sha256_try_start(&duff, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_ERROR_RESOURCE_IN_USE);
    rc = pico_sha256_start_blocking_until(&duff, SHA256_BIG_ENDIAN, use_dma, make_timeout_time_ms(100));
    hard_assert(rc == PICO_ERROR_TIMEOUT);

    pico_sha256_update_blocking(&state, nist_1, sizeof(nist_1));
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(nist_1_expected, result.bytes, SHA256_RESULT_BYTES) == 0);

    // Repeat with multiple calls
    rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, nist_1+0, 1);
    pico_sha256_update_blocking(&state, nist_1+1, 1);
    pico_sha256_update_blocking(&state, nist_1+2, 1);
    pico_sha256_finish(&state, &result);
    hard_assert(memcmp(nist_1_expected, result.bytes, SHA256_RESULT_BYTES) == 0);

    // Test different size of buffer for hardware "not ready" errors
    memset(buffer, 0, 1024);
    for(int i=0; i <= 1024; i++) {
        rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, use_dma);
        hard_assert(rc == PICO_OK);
        pico_sha256_update(&state, buffer, i);
        pico_sha256_finish(&state, &result);
    }
    free(buffer);
}

int main() {
    stdio_init_all();

    run_test(false);
    run_test(true);

    printf("Test passed\n");
}
