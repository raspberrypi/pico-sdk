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

#define RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE 32
#define RPI_CONNECT_CRYPTO_ECDSA_P256_SIG_MAX_SIZE 72

int rpi_connect_crypto_sha256(const char *data, size_t data_len, unsigned char *out_hash, size_t *out_len);
int rpi_connect_crypto_hmac_sha256(const char *data, size_t data_len, const char *key, size_t key_len, unsigned char *out_hmac, size_t *out_len);

//
// Sign a pre-computed SHA-256 hash with an ECDSA P-256 private key.
//
// hash:        32-byte SHA-256 digest to sign.
// private_key: 32-byte raw P-256 private key (big-endian scalar).
// out_sig:     Buffer for DER-encoded ECDSA signature.
// out_sig_len: On entry, buffer size (>= RPI_CONNECT_CRYPTO_ECDSA_P256_SIG_MAX_SIZE).
// On exit, actual signature length.
//
// Returns 0 on success, non-zero on failure.
int rpi_connect_crypto_ecdsa_p256_sign(const unsigned char hash[RPI_CONNECT_SHA256_SIZE],
                                       const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE],
                                       unsigned char *out_sig, size_t *out_sig_len);

//
// Derive the P-256 public key from a raw 32-byte private key scalar and
// return it as a PEM string (caller frees).  Returns NULL on failure.
char *rpi_connect_crypto_ecdsa_p256_pubkey_pem(
    const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif // _CONNECT_CRYPTO_H
