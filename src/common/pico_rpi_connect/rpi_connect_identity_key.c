/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Device implementation of the device identity key functions. These are the only functions which
// handle the private key.

#include <stdlib.h>
#include <string.h>

#include "pico/rpi_connect_identity.h"

#include "mbedtls/ecdsa.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/platform_util.h"

#if !PICO_RP2040
#include "hardware/regs/addressmap.h"
#endif

#if __has_include("device_identity_keys.h")
#include "device_identity_keys.h"
#endif

#define PRIVKEY_SIZE 32

#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    for (size_t i = 0; i < out_len * 2; i++) {
        char c = hex[i];
        uint8_t nibble;
        if (c >= '0' && c <= '9') nibble = (uint8_t)(c - '0');
        else if (c >= 'a' && c <= 'f') nibble = (uint8_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') nibble = (uint8_t)(c - 'A' + 10);
        else return -1;
        out[i / 2] = (uint8_t)((out[i / 2] << 4) | nibble);
    }
    return hex[out_len * 2] == '\0' ? 0 : -1;
}
#endif

// Read the private key. A build-time debug key overrides the OTP key.
// Returns 0 if a non-zero key was read; the caller must zeroize the key after use.
static int read_private_key(uint8_t key[PRIVKEY_SIZE]) {
    memset(key, 0, PRIVKEY_SIZE);
#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
    if (hex_to_bytes(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX, key, PRIVKEY_SIZE) != 0)
        memset(key, 0, PRIVKEY_SIZE);
#elif !PICO_RP2040
    // 16 ECC rows of 2 bytes, read through the guarded (ECC corrected) alias
    const volatile uint16_t *otp_data = (const volatile uint16_t *)OTP_DATA_GUARDED_BASE;
    for (unsigned int i = 0; i < PRIVKEY_SIZE / 2; i++) {
        uint16_t row = otp_data[RPI_CONNECT_IDENTITY_OTP_ROW + i];
        key[i * 2] = (uint8_t)row;
        key[i * 2 + 1] = (uint8_t)(row >> 8);
    }
#endif
    uint8_t any = 0;
    for (size_t i = 0; i < PRIVKEY_SIZE; i++)
        any |= key[i];
    return any ? 0 : -1;
}

bool rpi_connect_identity_key_available(void) {
    uint8_t key[PRIVKEY_SIZE];
    bool available = read_private_key(key) == 0;
    mbedtls_platform_zeroize(key, sizeof(key));
    return available;
}

// Load the private key into an ECP keypair, with an RNG for blinding and signing. The contexts are
// over 1kB, so are allocated on the heap rather than the stack, which may be small.
typedef struct {
    mbedtls_ecp_keypair keypair;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
} identity_key_ctx_t;

static void identity_key_free(identity_key_ctx_t *ctx) {
    if (!ctx)
        return;
    mbedtls_ecp_keypair_free(&ctx->keypair);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_platform_zeroize(ctx, sizeof(*ctx));
    free(ctx);
}

static identity_key_ctx_t *identity_key_load(void) {
    identity_key_ctx_t *ctx = malloc(sizeof(*ctx));
    if (!ctx)
        return NULL;
    mbedtls_ecp_keypair_init(&ctx->keypair);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_entropy_init(&ctx->entropy);

    int ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, NULL, 0);
    if (ret == 0) {
        uint8_t key[PRIVKEY_SIZE];
        ret = read_private_key(key);
        if (ret == 0)
            ret = mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256R1, &ctx->keypair, key, sizeof(key));
        mbedtls_platform_zeroize(key, sizeof(key));
    }
    if (ret != 0) {
        identity_key_free(ctx);
        return NULL;
    }
    return ctx;
}

int rpi_connect_identity_get_public_key(uint8_t out_pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE]) {
    identity_key_ctx_t *ctx = identity_key_load();
    if (!ctx)
        return -1;
    int ret = mbedtls_ecp_keypair_calc_public(&ctx->keypair, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    if (ret == 0) {
        size_t len = 0;
        ret = mbedtls_ecp_write_public_key(&ctx->keypair, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                           &len, out_pubkey, RPI_CONNECT_IDENTITY_PUBKEY_SIZE);
        if (ret == 0 && len != RPI_CONNECT_IDENTITY_PUBKEY_SIZE)
            ret = -1;
    }
    identity_key_free(ctx);
    return ret == 0 ? 0 : -1;
}

int rpi_connect_identity_sign_hash(const uint8_t hash[RPI_CONNECT_IDENTITY_HASH_SIZE],
                                   uint8_t *out_sig, size_t *out_sig_len) {
    if (!hash || !out_sig || !out_sig_len)
        return -1;

    identity_key_ctx_t *ctx = identity_key_load();
    if (!ctx)
        return -1;
    // An mbedtls_ecdsa_context is an mbedtls_ecp_keypair, so the keypair can be used directly
    size_t sig_len = 0;
    int ret = mbedtls_ecdsa_write_signature(&ctx->keypair, MBEDTLS_MD_SHA256,
                                            hash, RPI_CONNECT_IDENTITY_HASH_SIZE,
                                            out_sig, *out_sig_len, &sig_len,
                                            mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    if (ret == 0)
        *out_sig_len = sig_len;
    identity_key_free(ctx);
    return ret == 0 ? 0 : -1;
}
