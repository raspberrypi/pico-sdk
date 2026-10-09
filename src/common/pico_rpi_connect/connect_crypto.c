/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/rpi_connect/internal/connect_crypto.h"
#include "pico/rpi_connect_util.h"

#if !PICO_ON_DEVICE
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

int rpi_connect_crypto_sha256(const char *data, size_t data_len, unsigned char *out_hash, size_t *out_len) {
    EVP_MD_CTX *mdctx = NULL;
    const EVP_MD *md;
    int ret = -1;

    md = EVP_sha256();
    if (md == NULL) {
        RPI_CONNECT_ERROR("Error getting SHA256 digest\n");
        goto cleanup;
    }

    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        RPI_CONNECT_ERROR("Error creating MD context\n");
        goto cleanup;
    }

    if (1 != EVP_DigestInit_ex(mdctx, md, NULL)) {
        RPI_CONNECT_ERROR("Error initializing digest\n");
        goto cleanup;
    }

    if (1 != EVP_DigestUpdate(mdctx, data, data_len)) {
        RPI_CONNECT_ERROR("Error updating digest\n");
        goto cleanup;
    }

    unsigned int hash_len;
    if (1 != EVP_DigestFinal_ex(mdctx, out_hash, &hash_len)) {
        RPI_CONNECT_ERROR("Error finalizing digest\n");
        goto cleanup;
    }
    *out_len = hash_len;

    ret = 0;

cleanup:
    if (mdctx) {
        EVP_MD_CTX_free(mdctx);
    }
    return ret;
}

int rpi_connect_crypto_hmac_sha256(const char *data, size_t data_len, const char *key, size_t key_len, unsigned char *out_hmac, size_t *out_len) {
    EVP_MD_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    const EVP_MD *md;
    size_t sig_len;
    int ret = -1;

    // Create the Message Digest Context
    if(!(ctx = EVP_MD_CTX_new())) {
        RPI_CONNECT_ERROR("Error creating context\n");
        goto cleanup;
    }

    // Create the key
    if(!(pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, (unsigned char *)key, key_len))) {
        RPI_CONNECT_ERROR("Error creating key\n");
        goto cleanup;
    }

    // Initialize the digest
    md = EVP_sha256();
    if(1 != EVP_DigestSignInit(ctx, NULL, md, NULL, pkey)) {
        RPI_CONNECT_ERROR("Error initializing digest sign\n");
        goto cleanup;
    }

    // Add data to be signed
    if(1 != EVP_DigestSignUpdate(ctx, data, data_len)) {
        RPI_CONNECT_ERROR("Error updating digest sign\n");
        goto cleanup;
    }

    // First call to get required buffer length
    if(1 != EVP_DigestSignFinal(ctx, NULL, &sig_len)) {
        RPI_CONNECT_ERROR("Error getting signature length\n");
        goto cleanup;
    }

    // Compute the HMAC
    if(1 != EVP_DigestSignFinal(ctx, out_hmac, &sig_len)) {
        RPI_CONNECT_ERROR("Error computing final digest\n");
        goto cleanup;
    }

    *out_len = sig_len;
    ret = 0;

cleanup:
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    return ret;
}

int rpi_connect_sha256_init(rpi_connect_sha256_ctx_t *ctx) {
    ctx->impl = EVP_MD_CTX_new();
    if (!ctx->impl)
        return -1;
    if (1 != EVP_DigestInit_ex((EVP_MD_CTX *)ctx->impl, EVP_sha256(), NULL)) {
        EVP_MD_CTX_free((EVP_MD_CTX *)ctx->impl);
        ctx->impl = NULL;
        return -1;
    }
    return 0;
}

int rpi_connect_sha256_update(rpi_connect_sha256_ctx_t *ctx, const void *data, size_t data_len) {
    if (!ctx->impl)
        return -1;
    return (1 == EVP_DigestUpdate((EVP_MD_CTX *)ctx->impl, data, data_len)) ? 0 : -1;
}

int rpi_connect_sha256_finish(rpi_connect_sha256_ctx_t *ctx, unsigned char out_hash[RPI_CONNECT_SHA256_SIZE]) {
    if (!ctx->impl)
        return -1;
    unsigned int len;
    int ret = (1 == EVP_DigestFinal_ex((EVP_MD_CTX *)ctx->impl, out_hash, &len)) ? 0 : -1;
    EVP_MD_CTX_free((EVP_MD_CTX *)ctx->impl);
    ctx->impl = NULL;
    return ret;
}

void rpi_connect_sha256_abort(rpi_connect_sha256_ctx_t *ctx) {
    if (!ctx->impl)
        return;
    EVP_MD_CTX_free((EVP_MD_CTX *)ctx->impl);
    ctx->impl = NULL;
}

#else

#include "mbedtls/sha256.h"
#include "mbedtls/version.h"
#include "mbedtls/md.h"

int rpi_connect_crypto_sha256(const char *data, size_t data_len, unsigned char *out_hash, size_t *out_len) {
    if (!data || !out_hash || !out_len) {
        return -1;
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    int ret = mbedtls_sha256_starts(&ctx, 0);  // 0 for SHA256 (not SHA224)
    if (ret != 0) {
        return ret;
    }

    ret = mbedtls_sha256_update(&ctx, (const unsigned char *)data, data_len);
    if (ret != 0) {
        return ret;
    }

    ret = mbedtls_sha256_finish(&ctx, out_hash);
    mbedtls_sha256_free(&ctx);
    if (ret != 0) {
        return ret;
    }

    *out_len = 32;  // SHA256 always produces 32 bytes
    return 0;
}

int rpi_connect_crypto_hmac_sha256(const char *data, size_t data_len, const char *key, size_t key_len,
                                   unsigned char *out_hmac, size_t *out_len) {
    if (!data || !key || !out_hmac || !out_len) {
        return -1;
    }

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info;
    int ret = -1;

    mbedtls_md_init(&ctx);
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        goto cleanup;
    }

    ret = mbedtls_md_setup(&ctx, md_info, 1); // 1 for HMAC
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, key_len);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_md_hmac_update(&ctx, (const unsigned char *)data, data_len);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_md_hmac_finish(&ctx, out_hmac);
    if (ret != 0) {
        goto cleanup;
    }

    *out_len = mbedtls_md_get_size(md_info);
    ret = 0;

cleanup:
    mbedtls_md_free(&ctx);
    return ret;
}

int rpi_connect_sha256_init(rpi_connect_sha256_ctx_t *ctx) {
    mbedtls_sha256_context *sha_ctx = malloc(sizeof(mbedtls_sha256_context));
    if (!sha_ctx)
        return -1;
    mbedtls_sha256_init(sha_ctx);
    int ret = mbedtls_sha256_starts(sha_ctx, 0); // 0 = SHA-256 (not SHA-224)
    if (ret != 0) {
        mbedtls_sha256_free(sha_ctx);
        free(sha_ctx);
        return ret;
    }
    ctx->impl = sha_ctx;
    return 0;
}

int rpi_connect_sha256_update(rpi_connect_sha256_ctx_t *ctx, const void *data, size_t data_len) {
    if (!ctx->impl)
        return -1;
    return mbedtls_sha256_update((mbedtls_sha256_context *)ctx->impl,
                                 (const unsigned char *)data, data_len);
}

int rpi_connect_sha256_finish(rpi_connect_sha256_ctx_t *ctx, unsigned char out_hash[RPI_CONNECT_SHA256_SIZE]) {
    if (!ctx->impl)
        return -1;
    mbedtls_sha256_context *sha_ctx = (mbedtls_sha256_context *)ctx->impl;
    int ret = mbedtls_sha256_finish(sha_ctx, out_hash);
    mbedtls_sha256_free(sha_ctx);
    free(sha_ctx);
    ctx->impl = NULL;
    return ret;
}

void rpi_connect_sha256_abort(rpi_connect_sha256_ctx_t *ctx) {
    if (!ctx->impl)
        return;
    mbedtls_sha256_context *sha_ctx = (mbedtls_sha256_context *)ctx->impl;
    mbedtls_sha256_free(sha_ctx);
    free(sha_ctx);
    ctx->impl = NULL;
}

#endif
