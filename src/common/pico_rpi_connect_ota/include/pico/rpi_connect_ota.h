/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_RPI_CONNECT_OTA_H
#define _PICO_RPI_CONNECT_OTA_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "pico/rpi_connect.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/rpi_connect_ota.h
 *  \defgroup pico_rpi_connect_ota pico_rpi_connect_ota
 *  \brief OTA deployment state machine for Raspberry Pi Connect
 *
 * Downloads a deployment artefact into flash with SHA-256 verification,
 * reboots into it via the bootrom's try-before-you-buy mechanism and
 * reports the outcome to the server. Deployment state is persisted in FFS
 * so it survives the reboot that applies an update.
 */

// OTA logging --------------------------------------------------------------
// The OTA layer has its own logging switches, independent of the core
// RPI_CONNECT_* logging, so OTA/deployment traffic can be enabled or silenced
// on its own. All levels are opt-in and default off in the library.
// Applications (e.g. pico_rpi_connect_test) turn them on by defining the
// *_ENABLE flags in their build. Output goes through RPI_CONNECT_PRINTF
// (from pico/rpi_connect_util.h) so it can be redirected.
#ifndef RPI_CONNECT_OTA_DEBUG_ENABLE
#define RPI_CONNECT_OTA_DEBUG_ENABLE 0
#endif

#ifndef RPI_CONNECT_OTA_INFO_ENABLE
#define RPI_CONNECT_OTA_INFO_ENABLE 0
#endif

#ifndef RPI_CONNECT_OTA_ERROR_ENABLE
#define RPI_CONNECT_OTA_ERROR_ENABLE 0
#endif

#if RPI_CONNECT_OTA_DEBUG_ENABLE
#define RPI_CONNECT_OTA_DEBUG(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_OTA_DEBUG(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while (0)
#endif

#if RPI_CONNECT_OTA_INFO_ENABLE
#define RPI_CONNECT_OTA_INFO(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_OTA_INFO(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while (0)
#endif

#if RPI_CONNECT_OTA_ERROR_ENABLE
#define RPI_CONNECT_OTA_ERROR(...) RPI_CONNECT_PRINTF(__VA_ARGS__)
#else
#define RPI_CONNECT_OTA_ERROR(...) do { if (0) { RPI_CONNECT_PRINTF(__VA_ARGS__); } } while (0)
#endif

// FFS file ids used for persisted OTA state
#define RPI_CONNECT_FFS_AUTH_TOKEN 0x1
#define RPI_CONNECT_FFS_DEPLOYMENT_ID 0x2
#define RPI_CONNECT_FFS_DEPLOYMENT_URI 0x3
#define RPI_CONNECT_FFS_DEPLOYMENT_CHECKSUM 0x4
#define RPI_CONNECT_FFS_DEPLOYMENT_STATUS 0x5

/*! \brief OTA deployment lifecycle, persisted in FFS
 *  \ingroup pico_rpi_connect_ota
 *
 * IDLE -> DOWNLOADING -> APPLYING -> IDLE. An absent FFS entry means IDLE.
 * Success is only reported to the server once the new image has booted and
 * signed in, i.e. on the boot after APPLYING.
 */
typedef enum {
    RPI_CONNECT_OTA_DEPLOYMENT_IDLE = 0,     ///< nothing in progress (FFS entry absent)
    RPI_CONNECT_OTA_DEPLOYMENT_DOWNLOADING,  ///< image is being downloaded into flash
    RPI_CONNECT_OTA_DEPLOYMENT_APPLYING,     ///< image downloaded + verified, rebooting to apply
} rpi_connect_ota_deployment_status_t;

/*! \brief Initialise OTA state and sign in
 *  \ingroup pico_rpi_connect_ota
 *
 * Tries, in priority order: a token in FFS (cached from a previous exchange,
 * or provisioned - see the --connect-token option of
 * partition_pico2_for_ffs.sh); the build-time PEM identity key
 * (RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX, debug override); the OTP
 * identity key. The latter two perform a device-identity-exchange
 * (POST /client/device-identity-exchange) using hostname for the registered
 * device name.
 *
 * \param client_id Connect client UUID
 * \param serial_number device serial number
 * \param hostname registered device name
 * \return 0 on success (an active token is available via
 *         rpi_connect_ota_get_auth_token), non-zero otherwise
 */
int rpi_connect_ota_init(const char *client_id, const char *serial_number,
                         const char *hostname);

// SHA-256 hex digest is 64 chars + NUL
#define RPI_CONNECT_OTA_SHA256_HEX_SIZE 65

/*! \brief Result of a completed download
 *  \ingroup pico_rpi_connect_ota
 */
typedef struct {
    size_t total_bytes;
    char sha256_hex[RPI_CONNECT_OTA_SHA256_HEX_SIZE];
} rpi_connect_ota_download_result_t;

/*! \brief User data callback, called for each chunk of downloaded data
 *  \ingroup pico_rpi_connect_ota
 *
 * \param data chunk data
 * \param data_len chunk size in bytes
 * \param arg user argument
 * \return Non-zero to abort the download
 */
typedef int (*rpi_connect_ota_data_callback_t)(const void *data, size_t data_len, void *arg);

/*! \brief Download from uri, computing SHA-256 throughout
 *  \ingroup pico_rpi_connect_ota
 *
 * \param uri artefact URI
 * \param expected_checksum may be NULL to skip verification
 * \param user_callback may be NULL if the caller only wants the result
 * \param user_arg passed to user_callback
 * \param tls_no_verify when true the download host's TLS certificate is not
 *        verified and integrity rests entirely on the checksum. Needed when
 *        artefacts are hosted on servers that do not chain to the Pi Connect
 *        root CA. Applies to the download only; API requests are unaffected.
 *        (This applies to all the download/install_update variants below.)
 * \param result may be NULL if the caller does not need it
 * \return 0 on success, -1 on error
 */
int rpi_connect_ota_download(
    const char *uri,
    const char *expected_checksum,
    rpi_connect_ota_data_callback_t user_callback,
    void *user_arg,
    bool tls_no_verify,
    rpi_connect_ota_download_result_t *result);

/*! \brief Async OTA download context: start/poll/stop
 *  \ingroup pico_rpi_connect_ota
 *
 * The caller drives the download by calling async_context_poll() (on device)
 * followed by rpi_connect_ota_download_poll() in the main loop.
 */
typedef struct rpi_connect_ota_download_ctx rpi_connect_ota_download_ctx_t;

/*! \brief Start an async OTA download
 *  \ingroup pico_rpi_connect_ota
 * \return Download context, or NULL on failure
 */
rpi_connect_ota_download_ctx_t *rpi_connect_ota_download_start(
    const char *uri,
    const char *expected_checksum,
    rpi_connect_ota_data_callback_t user_callback,
    void *user_arg,
    bool tls_no_verify);

/*! \brief Poll an async OTA download
 *  \ingroup pico_rpi_connect_ota
 *
 * \param ctx download context
 * \param result filled on success if non-NULL
 * \return 0 = in progress, 1 = completed successfully (result is filled if
 *         non-NULL), -1 = error
 */
int rpi_connect_ota_download_poll(
    rpi_connect_ota_download_ctx_t *ctx,
    rpi_connect_ota_download_result_t *result);

/*! \brief Total size of the download in bytes (server Content-Length)
 *  \ingroup pico_rpi_connect_ota
 *
 * Valid once the response headers have been received (i.e. after at least
 * one poll/async_context_poll cycle following start). Combine with
 * rpi_connect_ota_download_bytes_so_far for progress reporting or to detect
 * the final chunk.
 *
 * \param ctx download context
 * \return The length, or 0 if not yet known or the server did not advertise
 *         one (e.g. chunked transfer encoding)
 */
size_t rpi_connect_ota_download_content_length(rpi_connect_ota_download_ctx_t *ctx);

/*! \brief Bytes received so far
 *  \ingroup pico_rpi_connect_ota
 *
 * Poll from the main loop alongside rpi_connect_ota_download_content_length
 * for progress indication.
 *
 * \param ctx download context
 */
size_t rpi_connect_ota_download_bytes_so_far(rpi_connect_ota_download_ctx_t *ctx);

/*! \brief Classify a failed download
 *  \ingroup pico_rpi_connect_ota
 *
 * Only meaningful once the download has finished with an error.
 *
 * \param ctx download context
 * \return True if the failure was transport-level (connection refused, DNS
 *         failure, dropped connection) and is assumed transient: keep the
 *         deployment and retry later. False if the server responded with an
 *         explicit HTTP error (e.g. 404) or the image failed checksum
 *         verification: the artefact is not retrievable as-is and the
 *         deployment should be failed.
 */
bool rpi_connect_ota_download_error_is_transient(rpi_connect_ota_download_ctx_t *ctx);

/*! \brief Stop an async download and free its resources
 *  \ingroup pico_rpi_connect_ota
 * \param ctx download context
 */
void rpi_connect_ota_download_stop(rpi_connect_ota_download_ctx_t *ctx);

/*! \brief Declare the client's supported capabilities (currently OTA only) via POST /me
 *  \ingroup pico_rpi_connect_ota
 *
 * The declaration is authoritative: capabilities this client does not
 * support are marked unavailable.
 *
 * \param token access token
 */
int rpi_connect_ota_register_ota_capability(const char *token);

/*! \brief Get/store the active auth token, persisted in FFS
 *  \ingroup pico_rpi_connect_ota
 * \return get: the token (caller frees), or NULL if none
 */
char *rpi_connect_ota_get_auth_token(void);
int rpi_connect_ota_store_auth_token(const char *token);

/*! \brief Get/store the current deployment id, persisted in FFS
 *  \ingroup pico_rpi_connect_ota
 * \return get: the id (caller frees), or NULL if none
 */
char *rpi_connect_ota_get_deployment_id(void);
int rpi_connect_ota_store_deployment_id(const char *deployment_id);

/*! \brief Get/set the persisted deployment status
 *  \ingroup pico_rpi_connect_ota
 *
 * get returns IDLE when no status is stored; set with IDLE clears the entry.
 *
 * \param status new status; IDLE clears the entry
 */
rpi_connect_ota_deployment_status_t rpi_connect_ota_get_deployment_status(void);
int rpi_connect_ota_set_deployment_status(rpi_connect_ota_deployment_status_t status);

/*! \brief Transition the deployment to in_progress on the server
 *  \ingroup pico_rpi_connect_ota
 *
 * Returns the artefact uri and checksum (caller frees both). The Connect API
 * always supplies a SHA-256 checksum; a response missing the uri or checksum
 * is treated as an error and the deployment is failed.
 *
 * \param token access token
 * \param deployment_id deployment to start or resume
 * \param uri receives the artefact uri (caller frees)
 * \param checksum receives the artefact checksum (caller frees)
 */
int rpi_connect_ota_start_deployment(const char *token, const char *deployment_id, char **uri, char **checksum);

/*! \brief Resume an interrupted deployment
 *  \ingroup pico_rpi_connect_ota
 *
 * Returns the artefact uri and checksum persisted in FFS, falling back to
 * rpi_connect_ota_start_deployment when they are absent.
 */
int rpi_connect_ota_resume_deployment(const char *token, const char *deployment_id, char **uri, char **checksum);

/*! \brief Report the deployment outcome to the server and clear the locally stored state
 *  \ingroup pico_rpi_connect_ota
 *
 * A terminal 404/422 response (deployment deleted, cancelled or superseded
 * server-side) also clears local state and returns success - retrying it can
 * never succeed and the device would otherwise retry a dead deployment on
 * every boot. Only a transport failure or a retryable server error keeps the
 * local state and returns an error.
 *
 * \param token access token
 * \param deployment_id deployment to report
 * \param reason failure reason reported to the server
 */
int rpi_connect_ota_complete_deployment(const char *token, const char *deployment_id);
int rpi_connect_ota_fail_deployment(const char *token, const char *deployment_id, const char *reason);

// Attempts (and delay between them) for the server calls made from
// rpi_connect_ota_boot_sync. Override at build time if needed.
#ifndef RPI_CONNECT_OTA_SERVER_RETRIES
#define RPI_CONNECT_OTA_SERVER_RETRIES 3
#endif
#ifndef RPI_CONNECT_OTA_SERVER_RETRY_DELAY_MS
#define RPI_CONNECT_OTA_SERVER_RETRY_DELAY_MS 2000
#endif

/*! \brief Boot-time OTA reconciliation; call once after rpi_connect_ota_init
 *  \ingroup pico_rpi_connect_ota
 *
 * Registers the OTA capability (the first authenticated round-trip to the
 * server), then reports the outcome of a just-applied update - success on a
 * flash-update boot, failure on a normal boot where the update is not
 * running - and finally issues the bootrom "buy".
 *
 * \param token access token
 * \return 0 on success. On error the buy has NOT been issued: the caller
 *         should stop so the bootrom rolls back to the previous image on the
 *         next reset.
 */
int rpi_connect_ota_boot_sync(const char *token);

/*! \brief Id of a deployment interrupted before or during its download
 *  \ingroup pico_rpi_connect_ota
 *
 * A stored id with any status other than APPLYING. Clears a stale status
 * left with no id.
 *
 * \return The id (caller frees), or NULL if there is nothing to resume
 */
char *rpi_connect_ota_get_resumable_deployment_id(void);

/*! \brief Get the static workarea buffer used for flash-image operations
 *  \ingroup pico_rpi_connect_ota
 */
int rpi_connect_ota_get_workarea(uint8_t **buffer, size_t *size);

/*! \brief Download data callback that writes each chunk to flash via pico_flash_image
 *  \ingroup pico_rpi_connect_ota
 */
int rpi_connect_ota_flash_image_data_callback(const void *data, size_t data_len, void *arg);

/*! \brief Download and install an update from the given URI
 *  \ingroup pico_rpi_connect_ota
 *
 * On Pico, this writes to flash via pico_flash_image. On POSIX, this writes
 * to a local file.
 *
 * \param uri artefact URI
 * \param expected_checksum required and must be non-empty: it is the only
 *        integrity check on the download (this applies to all the
 *        install_update variants below)
 */
int rpi_connect_ota_install_update(const char *uri, const char *expected_checksum,
                                   bool tls_no_verify);

/*! \brief Async install-update context: start/poll/stop
 *  \ingroup pico_rpi_connect_ota
 *
 * The caller drives the update by calling async_context_poll() (on device)
 * followed by rpi_connect_ota_install_update_poll() in the main loop.
 */
typedef struct rpi_connect_ota_install_update_ctx rpi_connect_ota_install_update_ctx_t;

/*! \brief Start an async install update
 *  \ingroup pico_rpi_connect_ota
 * \return Update context, or NULL on failure
 */
rpi_connect_ota_install_update_ctx_t *rpi_connect_ota_install_update_start(
    const char *uri, const char *expected_checksum, bool tls_no_verify);

/*! \brief Poll an async install update
 *  \ingroup pico_rpi_connect_ota
 *
 * \param ctx update context
 * \param result filled on success if non-NULL
 * \return 0 = in progress, 1 = completed successfully (result is filled if
 *         non-NULL), -1 = error
 */
int rpi_connect_ota_install_update_poll(
    rpi_connect_ota_install_update_ctx_t *ctx,
    rpi_connect_ota_download_result_t *result);

/*! \brief Download progress: bytes written so far and the total download size
 *  \ingroup pico_rpi_connect_ota
 *
 * The total is 0 if the server did not advertise a Content-Length (e.g.
 * chunked transfer encoding). Poll from the main loop for progress
 * indication.
 *
 * \param ctx update context
 */
size_t rpi_connect_ota_install_update_bytes_so_far(rpi_connect_ota_install_update_ctx_t *ctx);
size_t rpi_connect_ota_install_update_content_length(rpi_connect_ota_install_update_ctx_t *ctx);

/*! \brief Install-update variant of rpi_connect_ota_download_error_is_transient
 *  \ingroup pico_rpi_connect_ota
 *
 * Query before rpi_connect_ota_install_update_stop.
 *
 * \param ctx update context
 */
bool rpi_connect_ota_install_update_error_is_transient(rpi_connect_ota_install_update_ctx_t *ctx);

/*! \brief Stop an install update and free its resources
 *  \ingroup pico_rpi_connect_ota
 * \param ctx update context
 */
void rpi_connect_ota_install_update_stop(rpi_connect_ota_install_update_ctx_t *ctx);

/*! \brief Commit the current deployment if the bootrom indicates the flash-update was successful
 *  \ingroup pico_rpi_connect_ota
 */
int rpi_connect_ota_handle_boot(void);

/*! \brief Reboot into a downloaded flash update (try-before-you-buy)
 *  \ingroup pico_rpi_connect_ota
 */
int rpi_connect_ota_try_booting_to_flash_update(void);

/*! \brief True if this is a flash-update (try-before-you-buy) boot
 *  \ingroup pico_rpi_connect_ota
 *
 * The running image is a freshly applied update awaiting the buy. False on a
 * normal boot, including after the bootrom has rolled back a failed update.
 */
bool rpi_connect_ota_boot_is_flash_update(void);

#if PICO_ON_DEVICE
#ifndef RPI_CONNECT_IDENTITY_OTP_ROW
#define RPI_CONNECT_IDENTITY_OTP_ROW 0xc0
#endif
/*! \brief Read the 32-byte device identity private key from OTP
 *  \ingroup pico_rpi_connect_ota
 *
 * \param start_row first OTP row of the key
 * \param out_key receives the key
 */
int rpi_connect_ota_read_identity_key_otp(unsigned int start_row, unsigned char out_key[32]);

/*! \brief True if a non-zero device identity private key is programmed into OTP
 *  \ingroup pico_rpi_connect_ota
 *
 * The key is read from RPI_CONNECT_IDENTITY_OTP_ROW.
 */
bool rpi_connect_ota_identity_key_programmed(void);
#endif

/*! \brief Run a /client/device-identity-exchange against the Connect API
 *  \ingroup pico_rpi_connect_ota
 *
 * Uses the supplied raw 32-byte P-256 private key and matching PEM public
 * key.
 *
 * \param client_id Connect client UUID
 * \param serial_number device serial number
 * \param hostname used as device name when the identity has none set
 * \param private_key_32 32-byte raw P-256 private key
 * \param public_key_pem PEM-encoded public key matching a registered identity
 * \param out_token on success, set to a strdup() of the access token
 *                  (caller frees)
 * \return 0 on success, non-zero on failure
 */
int rpi_connect_ota_device_identity_exchange(
    const char *client_id,
    const char *serial_number,
    const char *hostname,
    const unsigned char *private_key_32,
    const char *public_key_pem,
    char **out_token);

/*! \brief Callback invoked from rpi_connect_ota_event_listen when a DEPLOY event is received
 *  \ingroup pico_rpi_connect_ota
 *
 * deployment_id points into the event payload and is only valid for the
 * duration of the call; copy it (e.g. strdup) if it's needed later.
 *
 * \param deployment_id id of the deployment to fetch
 * \param arg user argument
 */
typedef void (*rpi_connect_ota_deploy_callback_t)(const char *deployment_id, void *arg);

/*! \brief Wrapper around rpi_connect_event_listen that handles all event types internally
 *  \ingroup pico_rpi_connect_ota
 *
 * Invokes deploy_cb for each DEPLOY event (other event types are handled
 * with debug logging). Stop with rpi_connect_event_stop. Only one listener
 * is supported at a time.
 *
 * \param context async context to run the listener on
 * \param token access token
 * \param deploy_cb called for each DEPLOY event
 * \param deploy_arg passed to deploy_cb
 */
rpi_connect_event_context_t *rpi_connect_ota_event_listen(
    async_context_t *context,
    const char *token,
    rpi_connect_ota_deploy_callback_t deploy_cb,
    void *deploy_arg);

#ifdef __cplusplus
}
#endif

#endif // _PICO_RPI_CONNECT_OTA_H
