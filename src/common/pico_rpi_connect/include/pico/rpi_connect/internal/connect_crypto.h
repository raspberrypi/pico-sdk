/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONNECT_CRYPTO_H
#define _CONNECT_CRYPTO_H

#include <stddef.h>

#include "pico/rpi_connect_util.h"

#ifdef __cplusplus
extern "C" {
#endif

int rpi_connect_crypto_sha256(const char *data, size_t data_len, unsigned char *out_hash, size_t *out_len);
int rpi_connect_crypto_hmac_sha256(const char *data, size_t data_len, const char *key, size_t key_len, unsigned char *out_hmac, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif // _CONNECT_CRYPTO_H
