/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Device identity functions which don't depend on how the private key is held: the public key PEM,
// and building and signing device identity exchange requests. The host and device specific parts
// are in rpi_connect_identity_key_host.c and rpi_connect_identity_key.c.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/rpi_connect_identity.h"
#include "pico/rpi_connect/internal/connect_identity.h"

char *rpi_connect_identity_public_key_pem(void) {
    uint8_t pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE];
    if (rpi_connect_identity_get_public_key(pubkey) != 0)
        return NULL;
    char *pem = malloc(RPI_CONNECT_IDENTITY_PEM_SIZE);
    if (pem && rpi_connect_identity_pubkey_to_pem(pubkey, pem, RPI_CONNECT_IDENTITY_PEM_SIZE) != 0) {
        free(pem);
        pem = NULL;
    }
    return pem;
}

// Check str is a non-empty NUL-terminated string within a char array of size size, with no control characters
static bool valid_string(const char *str, size_t size) {
    const char *end = memchr(str, '\0', size);
    if (!end || end == str)
        return false;
    for (const char *p = str; p < end; p++) {
        if ((unsigned char)*p < 0x20 || *p == 0x7f)
            return false;
    }
    return true;
}

int rpi_connect_identity_exchange_validate(const rpi_connect_identity_exchange_t *exchange) {
    if (!exchange ||
        !valid_string(exchange->api_host, sizeof(exchange->api_host)) ||
        !valid_string(exchange->client_id, sizeof(exchange->client_id)) ||
        !valid_string(exchange->hostname, sizeof(exchange->hostname)) ||
        !valid_string(exchange->serial_number, sizeof(exchange->serial_number)))
        return -1;
    // The API host is part of the signed URL, so must be a plain host[:port]
    for (const char *p = exchange->api_host; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
              *p == '.' || *p == '-' || *p == ':'))
            return -1;
    }
    // The timestamp is required, so that the signed request is only valid for a short time
    if (exchange->timestamp <= 0)
        return -1;
    return 0;
}

typedef struct {
    char *buf;
    size_t size;
    size_t pos;
    bool overflow;
} str_builder_t;

static void append_char(str_builder_t *sb, char c) {
    if (sb->pos + 1 < sb->size)
        sb->buf[sb->pos++] = c;
    else
        sb->overflow = true;
}

static void append(str_builder_t *sb, const char *str) {
    while (*str)
        append_char(sb, *str++);
}

// Append a JSON string value (validated strings and the PEM only need quotes, backslashes and newlines escaped)
static void append_json_string(str_builder_t *sb, const char *str) {
    append_char(sb, '"');
    for (; *str; str++) {
        if (*str == '"' || *str == '\\') {
            append_char(sb, '\\');
            append_char(sb, *str);
        } else if (*str == '\n') {
            append(sb, "\\n");
        } else {
            append_char(sb, *str);
        }
    }
    append_char(sb, '"');
}

// Build the body, in the same form as cJSON_PrintUnformatted(). This is not inlined, so the PEM
// isn't on the stack while using the private key, as the stack may be small
static int __attribute__((noinline)) build_body(rpi_connect_identity_exchange_t *exchange,
                                                const uint8_t pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE],
                                                size_t *out_len) {
    char pem[RPI_CONNECT_IDENTITY_PEM_SIZE];
    if (rpi_connect_identity_pubkey_to_pem(pubkey, pem, sizeof(pem)) != 0)
        return -1;
    str_builder_t body = { .buf = exchange->body, .size = sizeof(exchange->body) };
    append(&body, "{\"client_id\":");
    append_json_string(&body, exchange->client_id);
    append(&body, ",\"public_key\":");
    append_json_string(&body, pem);
    append(&body, ",\"hostname\":");
    append_json_string(&body, exchange->hostname);
    append(&body, ",\"serial_number\":");
    append_json_string(&body, exchange->serial_number);
    append(&body, "}");
    if (body.overflow)
        return -1;
    body.buf[body.pos] = '\0';
    *out_len = body.pos;
    return 0;
}

int rpi_connect_identity_exchange_sign_validated(rpi_connect_identity_exchange_t *exchange) {
    // Get the public key before building the body, so the private key isn't used beneath build_body()
    uint8_t pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE];
    if (rpi_connect_identity_get_public_key(pubkey) != 0)
        return -1;
    size_t body_len;
    uint8_t hash[RPI_CONNECT_IDENTITY_HASH_SIZE];
    if (build_body(exchange, pubkey, &body_len) != 0 || rpi_connect_identity_sha256(exchange->body, body_len, hash) != 0)
        return -1;

    // The signed payload: method, URL, signed headers and body hash, as in rpi_connect.c
    char *payload = malloc(512);
    if (!payload)
        return -1;
    int len = snprintf(payload, 512,
                       "POST\nhttps://%s" RPI_CONNECT_IDENTITY_EXCHANGE_ENDPOINT "\n"
                       "Content-Type: application/json\n"
                       "Accept: */*\n"
                       "X-Connect-Timestamp: %lld\n",
                       exchange->api_host, (long long)exchange->timestamp);
    for (int i = 0; i < RPI_CONNECT_IDENTITY_HASH_SIZE && len > 0 && len + 2 < 512; i++)
        len += snprintf(payload + len, 512 - (size_t)len, "%02x", hash[i]);
    int ret = -1;
    if (len > 0 && len < 512)
        ret = rpi_connect_identity_sha256(payload, (size_t)len, hash);
    free(payload);
    if (ret != 0)
        return -1;

    size_t sig_len = sizeof(exchange->sig);
    if (rpi_connect_identity_sign_hash(hash, exchange->sig, &sig_len) != 0)
        return -1;
    exchange->sig_len = sig_len;
    return 0;
}

int rpi_connect_identity_sign_exchange(rpi_connect_identity_exchange_t *exchange) {
    if (rpi_connect_identity_exchange_validate(exchange) != 0)
        return -1;
    return rpi_connect_identity_exchange_sign_validated(exchange);
}

