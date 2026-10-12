/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONNECT_IDENTITY_H
#define _CONNECT_IDENTITY_H

#include "pico/rpi_connect_identity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPI_CONNECT_IDENTITY_EXCHANGE_ENDPOINT "/client/device-identity-exchange"

// Room for the PEM of a P-256 SubjectPublicKeyInfo (~180 bytes)
#define RPI_CONNECT_IDENTITY_PEM_SIZE 256

// Host or device specific (rpi_connect_identity_key_host.c or rpi_connect_identity_key.c)
// Write the PEM of a raw public key to out
int rpi_connect_identity_pubkey_to_pem(const uint8_t pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE], char *out, size_t size);
// SHA-256 hash of data
int rpi_connect_identity_sha256(const char *data, size_t len, uint8_t out[RPI_CONNECT_IDENTITY_HASH_SIZE]);

// Check the inputs of a device identity exchange request are valid, so the body can be built
int rpi_connect_identity_exchange_validate(const rpi_connect_identity_exchange_t *exchange);

// Build and sign a request which has already been validated with rpi_connect_identity_exchange_validate()
int rpi_connect_identity_exchange_sign_validated(rpi_connect_identity_exchange_t *exchange);

#ifdef __cplusplus
}
#endif

#endif
