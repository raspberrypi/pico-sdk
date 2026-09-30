/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RPI_CONNECT_UTIL_H
#define _PICO_RPI_CONNECT_UTIL_H

#include "pico/time.h"

#if !PICO_ON_DEVICE
// For test/debug RPi Connect can also be used with libcurl
#include <curl/curl.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
typedef void *async_context_t;
#define cyw43_arch_async_context() NULL
#else
#include "pico/async_context.h"
#include "pico/stdio.h"
struct curl_slist;
#endif

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rpi_connect_util.h
 *  \ingroup pico_rpi_connect
 *  \brief Platform support for the Pi Connect library: standard headers,
 *  logging macros and streaming SHA-256
 */

/*! \brief The async_context to use when the application does not supply one
 *  \ingroup pico_rpi_connect
 */
async_context_t *rpi_connect_default_async_context(void);

/*! \brief Output function for all Pi Connect library logging
 *  \ingroup pico_rpi_connect
 *
 * Every RPI_CONNECT_* and RPI_CONNECT_OTA_* logging macro expands to
 * RPI_CONNECT_PRINTF, which defaults to printf and can be redefined in the
 * application build to redirect the output. All logging, including errors,
 * is compiled out by default: applications opt in per level by defining the
 * *_ENABLE flags in their build. */
#ifndef RPI_CONNECT_PRINTF
#define RPI_CONNECT_PRINTF printf
#endif

#ifndef RPI_CONNECT_DEBUG_ENABLE
#define RPI_CONNECT_DEBUG_ENABLE 0
#endif

#ifndef RPI_CONNECT_VERBOSE_DEBUG_ENABLE
#define RPI_CONNECT_VERBOSE_DEBUG_ENABLE 0
#endif

#ifndef RPI_CONNECT_INFO_ENABLE
#define RPI_CONNECT_INFO_ENABLE 0
#endif

#ifndef RPI_CONNECT_ERROR_ENABLE
#define RPI_CONNECT_ERROR_ENABLE 0
#endif

#if RPI_CONNECT_VERBOSE_DEBUG_ENABLE
#define RPI_CONNECT_VERBOSE_DEBUG(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_VERBOSE_DEBUG(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while(0)
#endif

#if RPI_CONNECT_DEBUG_ENABLE
#define RPI_CONNECT_DEBUG(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_DEBUG(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while(0)
#endif

#if RPI_CONNECT_INFO_ENABLE
#define RPI_CONNECT_INFO(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_INFO(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while(0)
#endif

#if RPI_CONNECT_ERROR_ENABLE
#define RPI_CONNECT_ERROR(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_ERROR(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while(0)
#endif

// SHA-256 digest size in bytes
#define RPI_CONNECT_SHA256_SIZE 32

/*! \brief Streaming SHA-256 context
 *  \ingroup pico_rpi_connect
 */
typedef struct {
    void *impl; ///< platform-specific context, managed by implementation
} rpi_connect_sha256_ctx_t;

/*! \brief Streaming SHA-256: init, add data, produce the digest
 *  \ingroup pico_rpi_connect
 *
 * \param ctx streaming context
 * \param data data to add
 * \param data_len size of data in bytes
 * \param out_hash receives the digest
 * \return 0 on success
 */
int rpi_connect_sha256_init(rpi_connect_sha256_ctx_t *ctx);
int rpi_connect_sha256_update(rpi_connect_sha256_ctx_t *ctx, const void *data, size_t data_len);
int rpi_connect_sha256_finish(rpi_connect_sha256_ctx_t *ctx, unsigned char out_hash[RPI_CONNECT_SHA256_SIZE]);

/*! \brief Discard a digest without producing a hash (e.g. an aborted download)
 *  \ingroup pico_rpi_connect
 *
 * Safe to call after finish or on an uninitialised-to-zero context.
 *
 * \param ctx streaming context
 */
void rpi_connect_sha256_abort(rpi_connect_sha256_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // _PICO_RPI_CONNECT_UTIL_H
