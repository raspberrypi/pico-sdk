/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RPI_CONNECT_IDENTITY_H
#define _PICO_RPI_CONNECT_IDENTITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rpi_connect_identity.h
 *  \defgroup pico_rpi_connect_identity pico_rpi_connect_identity
 *
 * \brief Raspberry Pi Connect device identity key
 *
 * The device identity is a P-256 key pair, used to sign the requests which register a device
 * identity and exchange it for a Connect access token. The private key is never exposed by this
 * API: only the public key can be read, and signing is performed by rpi_connect_identity_sign_exchange()
 * (or rpi_connect_identity_sign_hash(), where the caller is trusted with the key).
 *
 * On device, the private key is taken from a build-time debug key (`RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX`,
 * from a `device_identity_keys.h` header) if one is present, and otherwise from OTP, starting at
 * `RPI_CONNECT_IDENTITY_OTP_ROW`. The private key is only held in memory for the duration of each call.
 *
 * On the host, the private key is loaded from a PEM file with rpi_connect_identity_load_key_pem_file().
 */

#if PICO_ON_DEVICE && !PICO_RP2040
// PICO_CONFIG: RPI_CONNECT_IDENTITY_OTP_ROW, First OTP row of the 32-byte P-256 device identity private key, type=int, default=0xc0, group=pico_rpi_connect_identity
#ifndef RPI_CONNECT_IDENTITY_OTP_ROW
#define RPI_CONNECT_IDENTITY_OTP_ROW 0xc0
#endif
#endif

// Size of the raw public key: an uncompressed P-256 point (0x04 || X || Y)
#define RPI_CONNECT_IDENTITY_PUBKEY_SIZE 65
// Maximum size of a DER-encoded ECDSA P-256 signature
#define RPI_CONNECT_IDENTITY_SIG_MAX_SIZE 72
// Size of the hash signed by rpi_connect_identity_sign_hash() (SHA-256)
#define RPI_CONNECT_IDENTITY_HASH_SIZE 32

/*! \brief Check whether a device identity private key is available
 *  \ingroup pico_rpi_connect_identity
 *
 * \return true if a (non-zero) private key is available for signing
 */
bool rpi_connect_identity_key_available(void);

/*! \brief Get the device identity public key
 *  \ingroup pico_rpi_connect_identity
 *
 * \param out_pubkey receives the public key as an uncompressed P-256 point
 * \return 0 on success, non-zero on failure
 */
int rpi_connect_identity_get_public_key(uint8_t out_pubkey[RPI_CONNECT_IDENTITY_PUBKEY_SIZE]);

/*! \brief Get the device identity public key as a PEM string
 *  \ingroup pico_rpi_connect_identity
 *
 * \return the PEM-encoded SubjectPublicKeyInfo (caller frees), or NULL on failure
 */
char *rpi_connect_identity_public_key_pem(void);

// Maximum lengths (excluding the terminating NUL) of the rpi_connect_identity_exchange_t strings
#define RPI_CONNECT_IDENTITY_API_HOST_MAX      64
#define RPI_CONNECT_IDENTITY_CLIENT_ID_MAX     40
#define RPI_CONNECT_IDENTITY_HOSTNAME_MAX      64
#define RPI_CONNECT_IDENTITY_SERIAL_NUMBER_MAX 32
// Size of the rpi_connect_identity_exchange_t body buffer, which is large enough for any valid inputs
#define RPI_CONNECT_IDENTITY_EXCHANGE_BODY_SIZE 640

/*! \brief A device identity exchange request, for rpi_connect_identity_sign_exchange()
 *  \ingroup pico_rpi_connect_identity
 */
typedef struct {
    // Inputs: NUL-terminated strings
    char api_host[RPI_CONNECT_IDENTITY_API_HOST_MAX + 1];           ///< Connect API host the request is sent to
    char client_id[RPI_CONNECT_IDENTITY_CLIENT_ID_MAX + 1];         ///< Connect client UUID
    char hostname[RPI_CONNECT_IDENTITY_HOSTNAME_MAX + 1];           ///< used as device name when the identity has none set
    char serial_number[RPI_CONNECT_IDENTITY_SERIAL_NUMBER_MAX + 1]; ///< hex serial used to identify the device
    int64_t timestamp;                                              ///< X-Connect-Timestamp, in seconds since the Unix epoch
    // Outputs
    char body[RPI_CONNECT_IDENTITY_EXCHANGE_BODY_SIZE];             ///< JSON request body to POST (NUL-terminated)
    uint8_t sig[RPI_CONNECT_IDENTITY_SIG_MAX_SIZE];                 ///< DER-encoded ECDSA signature, for X-Connect-Identity-Signature
    size_t sig_len;                                                 ///< length of sig
} rpi_connect_identity_exchange_t;

/*! \brief Build and sign a device identity exchange request
 *  \ingroup pico_rpi_connect_identity
 *
 * Builds the JSON body for POST https://<api_host>/client/device-identity-exchange, containing the
 * client ID, the device identity public key, the hostname and the serial number, and signs the request
 * (method, URL, Content-Type, Accept and X-Connect-Timestamp headers, and body hash) with the device
 * identity private key. The caller must send exactly this body, with the headers
 * `Content-Type: application/json`, `Accept: &#42;/&#42;`, `X-Connect-Timestamp: <timestamp>`
 * and `X-Connect-Identity-Signature: <base64 of sig>`.
 *
 * \param exchange the request: the inputs are read, and the body and signature are written
 * \return 0 on success, non-zero on failure (including invalid inputs, or a timestamp which isn't positive)
 */
int rpi_connect_identity_sign_exchange(rpi_connect_identity_exchange_t *exchange);

/*! \brief Sign a SHA-256 hash with the device identity private key
 *  \ingroup pico_rpi_connect_identity
 *
 * \param hash the SHA-256 hash to sign
 * \param out_sig receives the DER-encoded ECDSA signature
 * \param out_sig_len on entry the size of out_sig (at least RPI_CONNECT_IDENTITY_SIG_MAX_SIZE),
 *                    on exit the length of the signature
 * \return 0 on success, non-zero on failure
 */
int rpi_connect_identity_sign_hash(const uint8_t hash[RPI_CONNECT_IDENTITY_HASH_SIZE],
                                   uint8_t *out_sig, size_t *out_sig_len);

#if !PICO_ON_DEVICE || PICO_COMBINED_DOCS
/*! \brief Load the device identity private key from a PEM file (host only)
 *  \ingroup pico_rpi_connect_identity
 *
 * \param filename path to an EC P-256 private key PEM file
 * \return 0 on success, non-zero on failure
 */
int rpi_connect_identity_load_key_pem_file(const char *filename);
#endif

#ifdef __cplusplus
}
#endif

#endif
