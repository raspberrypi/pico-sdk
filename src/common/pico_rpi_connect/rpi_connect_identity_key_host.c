/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Host implementation of the device identity key functions, over OpenSSL. The key is loaded from a
// PEM file with rpi_connect_identity_load_key_pem_file(), or from a build-time debug key.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/rpi_connect_identity.h"
#include "pico/rpi_connect_util.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/param_build.h>
#include <openssl/core_names.h>

#if __has_include("device_identity_keys.h")
#include "device_identity_keys.h"
#endif

#define PRIVKEY_SIZE 32

static EVP_PKEY *g_identity_key;

#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
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

    priv_bn = BN_bin2bn(private_key, PRIVKEY_SIZE, NULL);
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
    BN_clear_free(priv_bn);
    return pkey;
}

static int hex_to_bytes(const char *hex, unsigned char *out, size_t out_len) {
    if (strlen(hex) != out_len * 2)
        return -1;
    for (size_t i = 0; i < out_len; i++) {
        unsigned int byte;
        if (sscanf(&hex[i * 2], "%2x", &byte) != 1)
            return -1;
        out[i] = (unsigned char)byte;
    }
    return 0;
}
#endif

// The loaded key, or the build-time debug key if no key has been loaded
static EVP_PKEY *identity_key(void) {
#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
    if (!g_identity_key) {
        unsigned char key[PRIVKEY_SIZE];
        if (hex_to_bytes(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX, key, sizeof(key)) == 0)
            g_identity_key = evp_pkey_from_p256_privkey(key);
        OPENSSL_cleanse(key, sizeof(key));
    }
#endif
    return g_identity_key;
}

int rpi_connect_identity_load_key_pem_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        RPI_CONNECT_ERROR("Error: cannot open %s\n", filename);
        return -1;
    }
    EVP_PKEY *pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!pkey) {
        RPI_CONNECT_ERROR("Error: failed to parse private key PEM from %s\n", filename);
        return -1;
    }

    char group[32];
    if (!EVP_PKEY_is_a(pkey, "EC") ||
        !EVP_PKEY_get_utf8_string_param(pkey, OSSL_PKEY_PARAM_GROUP_NAME, group, sizeof(group), NULL) ||
        strcmp(group, "prime256v1") != 0) {
        RPI_CONNECT_ERROR("Error: key in %s is not an EC P-256 key\n", filename);
        EVP_PKEY_free(pkey);
        return -1;
    }

    EVP_PKEY_free(g_identity_key);
    g_identity_key = pkey;
    return 0;
}

bool rpi_connect_identity_key_available(void) {
    return identity_key() != NULL;
}

int rpi_connect_identity_get_public_key(uint8_t out_pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE]) {
    EVP_PKEY *pkey = identity_key();
    size_t len = 0;
    if (!pkey ||
        !EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY,
                                         out_pubkey, RPI_CONNECT_IDENTITY_PUBKEY_SIZE, &len) ||
        len != RPI_CONNECT_IDENTITY_PUBKEY_SIZE)
        return -1;
    return 0;
}

int rpi_connect_identity_sign_hash(const uint8_t hash[RPI_CONNECT_IDENTITY_HASH_SIZE],
                                   uint8_t *out_sig, size_t *out_sig_len) {
    EVP_PKEY *pkey = identity_key();
    if (!pkey || !hash || !out_sig || !out_sig_len)
        return -1;

    int ret = -1;
    EVP_PKEY_CTX *sign_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (sign_ctx && EVP_PKEY_sign_init(sign_ctx) > 0) {
        size_t sig_len = *out_sig_len;
        if (EVP_PKEY_sign(sign_ctx, out_sig, &sig_len, hash, RPI_CONNECT_IDENTITY_HASH_SIZE) > 0) {
            *out_sig_len = sig_len;
            ret = 0;
        }
    }
    EVP_PKEY_CTX_free(sign_ctx);
    return ret;
}
