/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "connect_crypto.h"
#include "pico/rpi_connect_util.h"

#if !PICO_ON_DEVICE
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/param_build.h>
#include <openssl/core_names.h>

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

// Build an EVP_PKEY keypair for P-256 from a raw 32-byte private key scalar.
static EVP_PKEY *evp_pkey_from_p256_privkey(const unsigned char *private_key) {
    BIGNUM *priv_bn = NULL;
    EC_GROUP *group = NULL;
    EC_POINT *pub_point = NULL;
    unsigned char *pub_oct = NULL;
    OSSL_PARAM_BLD *bld = NULL;
    OSSL_PARAM *params = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;

    priv_bn = BN_bin2bn(private_key, RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE, NULL);
    if (!priv_bn) goto cleanup;

    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    if (!group) goto cleanup;

    pub_point = EC_POINT_new(group);
    if (!pub_point) goto cleanup;

    if (1 != EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL))
        goto cleanup;

    size_t pub_oct_len = EC_POINT_point2oct(group, pub_point,
        POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
    pub_oct = malloc(pub_oct_len);
    if (!pub_oct) goto cleanup;
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED,
                       pub_oct, pub_oct_len, NULL);

    bld = OSSL_PARAM_BLD_new();
    if (!bld) goto cleanup;
    if (!OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0))
        goto cleanup;
    if (!OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PRIV_KEY, priv_bn))
        goto cleanup;
    if (!OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY, pub_oct, pub_oct_len))
        goto cleanup;

    params = OSSL_PARAM_BLD_to_param(bld);
    if (!params) goto cleanup;

    pctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
    if (!pctx) goto cleanup;
    if (EVP_PKEY_fromdata_init(pctx) <= 0) goto cleanup;
    if (EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_KEYPAIR, params) <= 0) goto cleanup;

cleanup:
    EVP_PKEY_CTX_free(pctx);
    OSSL_PARAM_free(params);
    OSSL_PARAM_BLD_free(bld);
    free(pub_oct);
    EC_POINT_free(pub_point);
    EC_GROUP_free(group);
    BN_free(priv_bn);
    return pkey;
}

int rpi_connect_crypto_ecdsa_p256_sign(const unsigned char hash[RPI_CONNECT_SHA256_SIZE],
                                       const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE],
                                       unsigned char *out_sig, size_t *out_sig_len) {
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *sign_ctx = NULL;
    int ret = -1;

    if (!hash || !private_key || !out_sig || !out_sig_len)
        return -1;

    pkey = evp_pkey_from_p256_privkey(private_key);
    if (!pkey) goto cleanup;

    sign_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!sign_ctx) goto cleanup;
    if (EVP_PKEY_sign_init(sign_ctx) <= 0) goto cleanup;

    size_t sig_len = *out_sig_len;
    if (EVP_PKEY_sign(sign_ctx, out_sig, &sig_len, hash, RPI_CONNECT_SHA256_SIZE) <= 0)
        goto cleanup;

    *out_sig_len = sig_len;
    ret = 0;

cleanup:
    EVP_PKEY_CTX_free(sign_ctx);
    EVP_PKEY_free(pkey);
    return ret;
}

char *rpi_connect_crypto_ecdsa_p256_pubkey_pem(
    const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE]) {
    EVP_PKEY *pkey = NULL;
    BIO *bio = NULL;
    char *pem = NULL;

    pkey = evp_pkey_from_p256_privkey(private_key);
    if (!pkey) goto cleanup;

    bio = BIO_new(BIO_s_mem());
    if (!bio) goto cleanup;
    if (1 != PEM_write_bio_PUBKEY(bio, pkey)) goto cleanup;

    long pem_len = BIO_get_mem_data(bio, NULL);
    char *bio_data;
    BIO_get_mem_data(bio, &bio_data);
    pem = malloc(pem_len + 1);
    if (pem) {
        memcpy(pem, bio_data, pem_len);
        pem[pem_len] = '\0';
    }

cleanup:
    BIO_free(bio);
    EVP_PKEY_free(pkey);
    return pem;
}

#else

#include "mbedtls/sha256.h"
#include "mbedtls/version.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

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

int rpi_connect_crypto_ecdsa_p256_sign(const unsigned char hash[RPI_CONNECT_SHA256_SIZE],
                                       const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE],
                                       unsigned char *out_sig, size_t *out_sig_len) {
    mbedtls_ecdsa_context ecdsa;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    size_t sig_len = 0;
    int ret = -1;

    if (!hash || !private_key || !out_sig || !out_sig_len)
        return -1;

    mbedtls_ecdsa_init(&ecdsa);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (ret != 0)
        goto cleanup;

    ret = mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256R1, &ecdsa,
                               private_key, RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE);
    if (ret != 0)
        goto cleanup;

    ret = mbedtls_ecdsa_write_signature(&ecdsa, MBEDTLS_MD_SHA256,
                                        hash, RPI_CONNECT_SHA256_SIZE,
                                        out_sig, *out_sig_len, &sig_len,
                                        mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0)
        goto cleanup;

    *out_sig_len = sig_len;
    ret = 0;

cleanup:
    mbedtls_ecdsa_free(&ecdsa);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

#include "mbedtls/ecp.h"
#include "mbedtls/base64.h"
#include "mbedtls/pk.h"

char *rpi_connect_crypto_ecdsa_p256_pubkey_pem(
    const unsigned char private_key[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE]) {
    mbedtls_pk_context pk;
    mbedtls_ecp_keypair *kp;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    char *pem = NULL;
    int ret;

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    if (ret != 0) goto cleanup;

    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0) goto cleanup;

    kp = mbedtls_pk_ec(pk);
    ret = mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256R1, kp,
                               private_key, RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE);
    if (ret != 0) goto cleanup;

    ret = mbedtls_ecp_keypair_calc_public(kp, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) goto cleanup;

    // SubjectPublicKeyInfo PEM is typically ~180 bytes
    size_t pem_size = 256;
    pem = malloc(pem_size);
    if (!pem) goto cleanup;

    ret = mbedtls_pk_write_pubkey_pem(&pk, (unsigned char *)pem, pem_size);
    if (ret != 0) {
        free(pem);
        pem = NULL;
    }

cleanup:
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return pem;
}

#endif
