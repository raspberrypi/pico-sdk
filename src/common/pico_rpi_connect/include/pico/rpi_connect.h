/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RPI_CONNECT_H
#define _PICO_RPI_CONNECT_H

#if !PICO_ON_DEVICE && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "pico/rpi_connect_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rpi_connect.h
 *  \defgroup pico_rpi_connect pico_rpi_connect
 *  \brief Client library for the Raspberry Pi Connect REST API
 *
 * Device registration and authentication, capability reporting, deployment
 * (OTA) queries, server time and asynchronous downloads. On device HTTPS
 * uses lwIP and mbedTLS; host builds use libcurl for test and debug.
 */

// Minimum HTTP request timeout accepted by rpi_connect_set_http_timeout(), in milliseconds
#define RPI_CONNECT_HTTP_TIMEOUT_MIN 500

// Opaque type for event context
typedef struct RPI_CONNECT_EVENT_CONTEXT rpi_connect_event_context_t;
struct curl_slist;

/*! \brief Packed capability value declaring OTA support
 *  \ingroup pico_rpi_connect
 *
 * One byte per feature in release order (vnc, shell, ota), lowest index
 * first. Within each byte, bit 0 = available, bit 1 = enabled.
 */
#define RPI_CONNECT_CAPABILITY_OTA 0x030000

/*! \brief Signin (device-code flow) response
 *  \ingroup pico_rpi_connect
 */
typedef struct {
    char *device_code;
    char *user_code;
    char *verification_uri_complete;
} t_rpi_connect_signin;

/*! \brief Start the device-code signin flow via POST /client/device
 *  \ingroup pico_rpi_connect
 *
 * \param client_id Connect client UUID
 * \param serial_number device serial number
 * \return Signin codes to present to the user (free with
 *         \ref rpi_connect_signin_cleanup), or NULL on error
 */
t_rpi_connect_signin *rpi_connect_signin(const char *client_id, const char *serial_number);

/*! \brief Free a signin response
 *  \ingroup pico_rpi_connect
 */
void rpi_connect_signin_cleanup(t_rpi_connect_signin *signin);

/*! \brief Poll for the access token after signin via POST /client/token
 *  \ingroup pico_rpi_connect
 *
 * \param client_id Connect client UUID
 * \param device_code device code from \ref rpi_connect_signin
 * \param serial_number device serial number
 * \return Access token (caller frees), or NULL while authorisation is pending
 */
char *rpi_connect_retrieve_token_with_device_code(const char *client_id, const char *device_code, const char *serial_number);

/*! \brief Declare the device's capabilities via POST /me
 *  \ingroup pico_rpi_connect
 *
 * All known features are sent explicitly - unsupported ones as
 * "unavailable" - so the client stays authoritative for the whole set. On
 * device, the Pico device information (platform, board, program, versions)
 * is sent in the same request.
 *
 * \param token access token
 * \param capability a packed RPI_CONNECT_CAPABILITY_* value
 * \return 0 on success
 */
int rpi_connect_send_capability(const char *token, unsigned int capability);

/*! \brief Exchange a provisioning auth key for an access token via
 *  POST /client/auth-key-exchange
 *  \ingroup pico_rpi_connect
 *
 * \param auth_key provisioning auth key
 * \param serial_number device serial number
 * \param hostname device name to register
 * \param client_id Connect client UUID
 * \return Access token (caller frees), or NULL on error
 */
char *rpi_connect_handle_auth_key(const char *auth_key, const char *serial_number, const char *hostname, const char *client_id);

/*! \brief Connect event types delivered by the SSE event stream
 *  \ingroup pico_rpi_connect
 */
typedef enum {
    RPI_CONNECT_EVENT_TYPE_DEPLOY,
    RPI_CONNECT_EVENT_TYPE_PING,
    RPI_CONNECT_EVENT_TYPE_WELCOME,
    RPI_CONNECT_EVENT_TYPE_CONFIRM_SUBSCRIPTION,
    RPI_CONNECT_EVENT_TYPE_UNKNOWN,
    RPI_CONNECT_EVENT_TYPE_MAX,
} rpi_connect_event_type_t;

/*! \brief A Connect event; string fields are valid only within the callback
 *  \ingroup pico_rpi_connect
 */
typedef struct CONNECT_EVENT {
    rpi_connect_event_type_t type;
    union {
        struct {
            const char *id;
        } deploy;
        struct {
            uint64_t timestamp;
        } ping;
        struct {
            const char *sid;
        } welcome;
        struct {
            const char *identifier;
        } confirm_subscription;
        struct {
            const char *event_type;
            const char *data;
        } unknown;
    } data;
} rpi_connect_event_t;

/*! \brief Callback invoked for each Connect event
 *  \ingroup pico_rpi_connect
 */
typedef void (*rpi_connect_event_callback_t)(const rpi_connect_event_t *event, void *arg);

typedef struct RPI_CONNECT_EVENT_CONTEXT rpi_connect_event_context_t;

/*! \brief Open a persistent connection listening for Connect events
 *  \ingroup pico_rpi_connect
 *
 * HTTP SSE on GET /events?channel=DeviceChannel against the event host.
 *
 * \param context async context to run the listener on
 * \param token access token
 * \param callback invoked for each event
 * \param callback_arg passed to callback
 * \return Event context, or NULL on failure
 */
rpi_connect_event_context_t *rpi_connect_event_listen(async_context_t *context, const char *token, rpi_connect_event_callback_t callback, void *callback_arg);

/*! \brief Poll for events, dispatching the callback for each outstanding event
 *  \ingroup pico_rpi_connect
 *
 * Not required on PICO_ON_DEVICE as async_context_poll() is used instead.
 *
 * \param context event context from \ref rpi_connect_event_listen
 */
int rpi_connect_event_poll(rpi_connect_event_context_t *context);

/*! \brief Stop the event listener and close the connection
 *  \ingroup pico_rpi_connect
 * \param context event context from \ref rpi_connect_event_listen
 */
void rpi_connect_event_stop(rpi_connect_event_context_t *context);

/*! \brief Get the Connect API and event hosts
 *  \ingroup pico_rpi_connect
 */
const char *rpi_connect_api_host(void);
const char *rpi_connect_event_host(void);

#if !PICO_ON_DEVICE
/*! \brief Test hook: point the API at a local mock server (host or host:port).
 *  \ingroup pico_rpi_connect
 * Host builds only; the hosts are fixed on device.
 */
void rpi_connect_set_api_host(const char *host);
#endif

/*! \brief Get/set the async_context used for network operations
 *  \ingroup pico_rpi_connect
 */
void rpi_connect_set_async_context(async_context_t *async_context);
async_context_t *rpi_connect_async_context(void);

/*! \brief Set the device serial number used for request signing
 *  \ingroup pico_rpi_connect
 *
 * The serial number is the HMAC key base for X-Connect-Signature on signed
 * endpoints (set/get/delete key, delta, etc). Must be set before any signed
 * API call if the device has a signing secret configured on the server side
 * (which it will have if registered via rpi-connectd's default flow).
 *
 * \param serial_number the serial number, or NULL to disable signing
 */
void rpi_connect_set_serial_number(const char *serial_number);
const char *rpi_connect_serial_number(void);

/*! \brief Get/set the Connect client UUID used for API calls
 *  \ingroup pico_rpi_connect
 */
void rpi_connect_set_client_id(const char *client_id);
const char *rpi_connect_client_id(void);

/*! \brief TLS configuration: get/set the CA certificate (PEM) used for API calls
 *  \ingroup pico_rpi_connect
 * \param ca_cert CA certificate, PEM format
 */
const char *rpi_connect_default_ca_cert(void);
const char *rpi_connect_ca_cert(void);
void rpi_connect_set_ca_cert(const char *ca_cert);

/*! \brief Timeout for synchronous HTTP requests
 *  \ingroup pico_rpi_connect
 * \param timeout_ms timeout in milliseconds, at least RPI_CONNECT_HTTP_TIMEOUT_MIN
 */
void rpi_connect_set_http_timeout(int timeout_ms);
uint32_t rpi_connect_http_timeout(void);

/*! \brief Fetch wall-clock time from the server via GET /up
 *  \ingroup pico_rpi_connect
 *
 * Wall-clock time (seconds since the Unix epoch) is needed for
 * X-Connect-Timestamp and there is no battery-backed RTC. Fetches the
 * current time from /up over plain HTTP (an X-Timestamp
 * header in epoch-seconds format) and keeps an offset from the boot clock.
 * Blocks for one HTTP round trip.
 *
 * \return The time, or 0 on failure
 */
int64_t rpi_connect_update_time(void);

/*! \brief Set the wall-clock time from another source (e.g. NTP)
 *  \ingroup pico_rpi_connect
 * \param unix_seconds a time observed "now", in seconds since the Unix epoch
 */
void rpi_connect_set_time(int64_t unix_seconds);

/*! \brief Current wall-clock time
 *  \ingroup pico_rpi_connect
 * \return Seconds since the Unix epoch, or 0 until a time has been set since boot
 */
int64_t rpi_connect_time(void);

/*! \brief Check for a pending deployment via GET /deployments/pending
 *  \ingroup pico_rpi_connect
 *
 * \param token access token
 * \param deployment_id set to the first pending deployment's id (caller
 *                      frees), or NULL if there is no pending deployment
 * \return 0 on success, non-zero on error
 */
int rpi_connect_get_pending_deployment(const char *token, char **deployment_id);

/*! \brief Transition a deployment to in_progress via POST /deployments/:id/start
 *  \ingroup pico_rpi_connect
 *
 * \param uri receives the artefact uri (caller frees)
 * \param checksum receives the artefact checksum (caller frees)
 * \return 0 on success, non-zero on error
 */
int rpi_connect_start_deployment(const char *token, const char *deployment_id, char **uri, char **checksum);

/*! \brief Report deployment success via POST /deployments/:id/complete
 *  \ingroup pico_rpi_connect
 * \return 0 on success, non-zero on error
 */
int rpi_connect_complete_deployment(const char *token, const char *deployment_id);

/*! \brief Report deployment failure via POST /deployments/:id/fail
 *  \ingroup pico_rpi_connect
 * \param reason failure reason reported to the server
 * \return 0 on success, non-zero on error
 */
int rpi_connect_fail_deployment(const char *token, const char *deployment_id, const char *reason);

/*! \brief Register a device identity with an organisation via
 *  POST /organisation/device-identities
 *  \ingroup pico_rpi_connect
 *
 * \param org_token organisation token
 * \param private_key 32-byte raw P-256 private key
 * \param public_key_pem PEM-encoded public key to register
 * \param description description for the device identity
 * \param device_name optional device name, or NULL
 * \return The new identity's id (caller frees), or NULL on error
 */
char *rpi_connect_create_device_identity(
    const char *org_token,
    const unsigned char *private_key,
    const char *public_key_pem,
    const char *description,
    const char *device_name);

/*! \brief Exchange a registered device identity for a Connect access token
 *  \ingroup pico_rpi_connect
 *
 * POST /client/device-identity-exchange, signed with the device's P-256
 * private key (X-Connect-Identity-Signature).
 *
 * \param client_id       fixed Connect client UUID
 * \param private_key     32-byte raw P-256 private key matching the
 *                        registered identity's public_key
 * \param public_key_pem  PEM-encoded public key matching a registered identity
 * \param hostname        used as device name when the identity has none set
 * \param serial_number   32- or 64-bit hex serial used to identify the device
 * \param out_device_id   if non-NULL, set to a strdup() of the returned
 *                        device_id on success (caller frees)
 * \return A strdup() of the access token on success (caller frees), NULL on
 *         any failure
 */
char *rpi_connect_device_identity_exchange(
    const char *client_id,
    const unsigned char *private_key,
    const char *public_key_pem,
    const char *hostname,
    const char *serial_number,
    char **out_device_id);

/*! \brief Async download data callback: receives each chunk; NULL data + 0 len signals completion
 *  \ingroup pico_rpi_connect
 */
typedef int (*rpi_connect_download_callback_t)(const void *data, size_t data_len, void *arg);

/*! \brief Async download error callback: HTTP code, or -1 for connection errors
 *  \ingroup pico_rpi_connect
 */
typedef void (*rpi_connect_download_error_callback_t)(int error_code, void *arg);

typedef struct rpi_connect_download_context rpi_connect_download_context_t;

/*! \brief Start an asynchronous download from a URI
 *  \ingroup pico_rpi_connect
 *
 * \param context async context to run the download on
 * \param uri download source
 * \param callback data callback
 * \param error_callback error callback
 * \param callback_arg passed to both callbacks
 * \param tls_no_verify when true the server certificate is not verified:
 * the connection is encrypted but the download host is unauthenticated.
 * Only for downloads whose integrity is guaranteed by other means, e.g. a
 * checksum. Applies to this download only: concurrent API requests are
 * unaffected.
 * \return Download context, or NULL on failure
 */
rpi_connect_download_context_t *rpi_connect_async_download(
    async_context_t *context, const char *uri,
    rpi_connect_download_callback_t callback,
    rpi_connect_download_error_callback_t error_callback,
    void *callback_arg, bool tls_no_verify);

/*! \brief Poll for download progress, invoking the data/error callback as needed
 *  \ingroup pico_rpi_connect
 *
 * Not required on PICO_ON_DEVICE as async_context_poll() is used instead.
 *
 * \param context download context
 * \return Non-zero when the download is complete
 */
int rpi_connect_download_poll(rpi_connect_download_context_t *context);

/*! \brief Stop a download and free its resources
 *  \ingroup pico_rpi_connect
 * \param context download context
 */
void rpi_connect_download_stop(rpi_connect_download_context_t *context);

/*! \brief Content-Length advertised by the server for a download, in bytes
 *  \ingroup pico_rpi_connect
 *
 * Valid once the response headers have been received (poll or
 * async_context_poll at least once after start).
 *
 * \param context download context
 * \return The length, or 0 if the server did not send a Content-Length
 *         (e.g. chunked transfer encoding) or the headers have not yet arrived
 */
size_t rpi_connect_download_content_length(rpi_connect_download_context_t *context);

#ifdef __cplusplus
}
#endif

#endif // _PICO_RPI_CONNECT_H
