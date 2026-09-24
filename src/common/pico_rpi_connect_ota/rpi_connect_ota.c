/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/rpi_connect_ota.h"
#include "pico/rpi_connect.h"
#include "pico/rpi_connect_util.h"
#include "connect_crypto.h"

#if PICO_ON_DEVICE
#include "boot/picoboot.h"
#include "pico/bootrom.h"
#include "pico/ffs.h"
#include "pico/flash_image.h"
#else
#include "pico/ffs.h"
#define PICO_FLASH_IMAGE_WORKAREA_SIZE 5248
#endif

#if __has_include("device_identity_keys.h")
#include "device_identity_keys.h"
#endif

static const char *g_client_id;
static const char *g_serial_number;
static char *g_active_auth_token;
static uint8_t g_ota_workarea[PICO_FLASH_IMAGE_WORKAREA_SIZE];

#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
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

int rpi_connect_ota_get_workarea(uint8_t **buffer, size_t *size) {
    if (!buffer || !size) {
        return -1;
    }
    *buffer = g_ota_workarea;
    *size = sizeof(g_ota_workarea);
    return 0;
}

static int ffs_update_string_with_error(uint8_t file_id, const char *data);

// Run a device-identity-exchange using the supplied raw P-256 private key.
// Derives the matching public key, calls the exchange, returns a strdup of
// the resulting access token (caller frees) or NULL on failure.
static char *sign_in_with_identity_key(const char *client_id,
                                       const char *serial_number,
                                       const char *hostname,
                                       const unsigned char *privkey) {
    char *pubkey_pem = rpi_connect_crypto_ecdsa_p256_pubkey_pem(privkey);
    if (!pubkey_pem) {
        RPI_CONNECT_OTA_ERROR("Failed to derive public key from private key\n");
        return NULL;
    }

    char *token = NULL;
    int rc = rpi_connect_ota_device_identity_exchange(
        client_id, serial_number, hostname, privkey, pubkey_pem, &token);
    free(pubkey_pem);

    if (rc != 0 || !token) {
        free(token);
        return NULL;
    }
    return token;
}

int rpi_connect_ota_init(const char *client_id, const char *serial_number,
                         const char *hostname) {
    g_client_id = client_id;
    g_serial_number = serial_number;

    free(g_active_auth_token);
    g_active_auth_token = NULL;

    int rc = ffs_initialise();
    if (rc != 0) {
        RPI_CONNECT_OTA_ERROR("Failed to initialise FFS (%d)\n", rc);
        return -1;
    }

    // Priority 1: FFS token, either cached by a previous boot or provisioned
    // (never built into the firmware image - tokens are secrets).
    char *ffs_token = ffs_get_string(RPI_CONNECT_FFS_AUTH_TOKEN);
    if (ffs_token) {
        RPI_CONNECT_OTA_DEBUG("Using auth token from FFS\n");
        g_active_auth_token = ffs_token;
        return 0;
    }

    // Priority 2 (debug): build-time PEM key overrides the OTP one.
    unsigned char privkey[RPI_CONNECT_CRYPTO_P256_PRIVKEY_SIZE];
    const char *key_source = NULL;

#if defined(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX)
    if (hex_to_bytes(RPI_CONNECT_DEVICE_IDENTITY_PRIVKEY_HEX, privkey, sizeof(privkey)) == 0) {
        key_source = "build-time PEM";
    }
#endif

    // Priority 3: OTP-programmed identity key.
#if PICO_ON_DEVICE
    if (!key_source && rpi_connect_ota_identity_key_programmed()) {
        rpi_connect_ota_read_identity_key_otp(RPI_CONNECT_IDENTITY_OTP_ROW, privkey);
        key_source = "OTP";
    }
#endif

    if (!key_source) {
        RPI_CONNECT_OTA_ERROR("No auth token and no identity key available\n");
        return 1;
    }

    RPI_CONNECT_OTA_DEBUG("Signing in with identity key from %s\n", key_source);
    g_active_auth_token = sign_in_with_identity_key(
        client_id, serial_number, hostname, privkey);
    if (!g_active_auth_token) {
        RPI_CONNECT_OTA_ERROR("Device identity exchange failed (%s)\n", key_source);
        return -1;
    }
    RPI_CONNECT_OTA_DEBUG("Caching new auth token in FFS\n");
    ffs_update_string_with_error(RPI_CONNECT_FFS_AUTH_TOKEN, g_active_auth_token);
    return 0;
}

static int ffs_update_string_with_error(uint8_t file_id, const char *data) {
    int rc = ffs_update_string(file_id, data);
    if (rc != 0) {
        RPI_CONNECT_OTA_ERROR("FFS: Failed to write file (%d)\n", rc);
        return -1;
    }
    return 0;
}

// Returns the active auth token chosen by rpi_connect_ota_init (caller frees),
// or NULL if init has not produced one.
char *rpi_connect_ota_get_auth_token(void) {
    return g_active_auth_token ? strdup(g_active_auth_token) : NULL;
}

int rpi_connect_ota_store_auth_token(const char *token) {
    return ffs_update_string_with_error(RPI_CONNECT_FFS_AUTH_TOKEN, token);
}

int rpi_connect_ota_store_deployment_id(const char *deployment_id) {
    return ffs_update_string_with_error(RPI_CONNECT_FFS_DEPLOYMENT_ID, deployment_id);
}

char *rpi_connect_ota_get_deployment_id(void) {
    return ffs_get_string(RPI_CONNECT_FFS_DEPLOYMENT_ID);
}

rpi_connect_ota_deployment_status_t rpi_connect_ota_get_deployment_status(void) {
    rpi_connect_ota_deployment_status_t status = RPI_CONNECT_OTA_DEPLOYMENT_IDLE;
    char *s = ffs_get_string(RPI_CONNECT_FFS_DEPLOYMENT_STATUS);
    if (s) {
        if (strcmp(s, "DOWNLOADING") == 0)
            status = RPI_CONNECT_OTA_DEPLOYMENT_DOWNLOADING;
        else if (strcmp(s, "APPLYING") == 0)
            status = RPI_CONNECT_OTA_DEPLOYMENT_APPLYING;
        free(s);
    }
    return status;
}

int rpi_connect_ota_set_deployment_status(rpi_connect_ota_deployment_status_t status) {
    const char *s = NULL;
    switch (status) {
        case RPI_CONNECT_OTA_DEPLOYMENT_DOWNLOADING: s = "DOWNLOADING"; break;
        case RPI_CONNECT_OTA_DEPLOYMENT_APPLYING:    s = "APPLYING";    break;
        case RPI_CONNECT_OTA_DEPLOYMENT_IDLE:
        default:
            break;
    }
    RPI_CONNECT_OTA_DEBUG("Deployment status -> %s\n", s ? s : "IDLE");
    if (!s) {
        // empty == IDLE
        ffs_delete(RPI_CONNECT_FFS_DEPLOYMENT_STATUS);
        return 0;
    }
    return ffs_update_string_with_error(RPI_CONNECT_FFS_DEPLOYMENT_STATUS, s);
}

int rpi_connect_ota_start_deployment(const char *token, const char *deployment_id, char **uri, char **checksum) {
    RPI_CONNECT_OTA_INFO("Starting deployment ID=%s\n", deployment_id);
    rpi_connect_ota_store_deployment_id(deployment_id);
    int rc = rpi_connect_start_deployment(token, deployment_id, uri, checksum);
    if (rc == 0) {
        // The Connect API always supplies the artefact URI and its SHA-256
        // checksum, and the checksum is the only integrity check on the
        // download (the artefact host's CA is not verified), so a deployment
        // without both is malformed: fail it rather than install unverified.
        if (!*uri || !**uri || !*checksum || !**checksum) {
            RPI_CONNECT_OTA_ERROR("Deployment ID=%s missing artefact uri or checksum\n", deployment_id);
            free(*uri);
            free(*checksum);
            *uri = NULL;
            *checksum = NULL;
            rpi_connect_ota_fail_deployment(token, deployment_id, "missing artefact uri or checksum");
            return -1;
        }
        ffs_update_string_with_error(RPI_CONNECT_FFS_DEPLOYMENT_URI, *uri);
        ffs_update_string_with_error(RPI_CONNECT_FFS_DEPLOYMENT_CHECKSUM, *checksum);
        rpi_connect_ota_set_deployment_status(RPI_CONNECT_OTA_DEPLOYMENT_DOWNLOADING);
    } else {
        RPI_CONNECT_OTA_DEBUG("Failed to start deployment for ID=%s (%d)\n", deployment_id, rc);
    }
    return rc;
}

int rpi_connect_ota_resume_deployment(const char *token, const char *deployment_id, char **uri, char **checksum) {
    RPI_CONNECT_OTA_INFO("Resuming deployment ID=%s\n", deployment_id);

    *uri = ffs_get_string(RPI_CONNECT_FFS_DEPLOYMENT_URI);
    *checksum = ffs_get_string(RPI_CONNECT_FFS_DEPLOYMENT_CHECKSUM);

    if (*uri && **uri && *checksum && **checksum)
        return 0;

    free(*uri);
    free(*checksum);
    *uri = NULL;
    *checksum = NULL;

    int rc = rpi_connect_ota_start_deployment(token, deployment_id, uri, checksum);
    if (rc != 0) {
        RPI_CONNECT_OTA_ERROR("Failed to restart deployment (%d)\n", rc);
        rpi_connect_ota_fail_deployment(token, deployment_id, "Failed to restart deployment");
    }
    return rc;
}

// The deployment endpoints return -http_code for an explicit HTTP error and a
// positive code for a transport failure. A 404 (deployment deleted) or 422
// (already in a terminal state server-side) can never succeed on retry, so
// keeping the local state would retry a dead deployment on every boot forever.
static bool deployment_error_is_terminal(int rc) {
    return rc == -404 || rc == -422;
}

static void deployment_clear_local_state(void) {
    ffs_delete(RPI_CONNECT_FFS_DEPLOYMENT_ID);
    ffs_delete(RPI_CONNECT_FFS_DEPLOYMENT_URI);
    ffs_delete(RPI_CONNECT_FFS_DEPLOYMENT_CHECKSUM);
    rpi_connect_ota_set_deployment_status(RPI_CONNECT_OTA_DEPLOYMENT_IDLE);
}

int rpi_connect_ota_complete_deployment(const char *token, const char *deployment_id) {
    int rc;
    RPI_CONNECT_OTA_INFO("Completing deployment ID=%s\n", deployment_id);
    rc = rpi_connect_complete_deployment(token, deployment_id);
    if (rc != 0 && !deployment_error_is_terminal(rc)) {
        RPI_CONNECT_OTA_ERROR("Failed to signal completion of deployment (%d)\n", rc);
        return -1;
    }
    if (rc != 0) {
        // The deployment was cancelled, superseded or deleted server-side.
        // Treat it as resolved: returning success here means boot_sync still
        // commits ("buys") the image - which demonstrably boots and reaches
        // the server - rather than rolling it back over server bookkeeping.
        RPI_CONNECT_OTA_ERROR("Deployment ID=%s terminal on server (%d); clearing local state\n", deployment_id, rc);
    }
    deployment_clear_local_state();

    return 0;
}

int rpi_connect_ota_fail_deployment(const char *token, const char *deployment_id, const char *reason) {
    int rc;
    RPI_CONNECT_OTA_ERROR("Failing deployment ID=%s reason=%s\n", deployment_id, reason ? reason : "");
    rc = rpi_connect_fail_deployment(token, deployment_id, reason);
    if (rc != 0 && !deployment_error_is_terminal(rc)) {
        RPI_CONNECT_OTA_ERROR("Failed to signal failure of deployment (%d)\n", rc);
        return -1;
    }
    if (rc != 0)
        RPI_CONNECT_OTA_ERROR("Deployment ID=%s terminal on server (%d); clearing local state\n", deployment_id, rc);
    deployment_clear_local_state();

    return 0;
}

int rpi_connect_ota_boot_sync(const char *token) {
    // Register the OTA capability first: the first authenticated round-trip to
    // the server, which gates the deferred success report and the "buy" below.
    int retries = RPI_CONNECT_OTA_SERVER_RETRIES;
    int rc = rpi_connect_ota_register_ota_capability(token);
    while (retries-- && rc != 0) {
        RPI_CONNECT_OTA_DEBUG("Failed to register OTA capability (%d) retries %d\n", rc, retries);
        sleep_ms(RPI_CONNECT_OTA_SERVER_RETRY_DELAY_MS);
        rc = rpi_connect_ota_register_ota_capability(token);
    }
    if (rc != 0) {
        RPI_CONNECT_OTA_ERROR("Failed to register OTA capability (%d)\n", rc);
        return rc;
    }

    // Status APPLYING on a flash-update boot: the freshly applied image has now
    // signed in and registered its capability, so report success - just before
    // the bootrom "buy" below. On a normal boot the update is not running (the
    // reboot never happened, or the bootrom rolled back to this old image), so
    // report failure instead and let the server re-deploy.
    if (rpi_connect_ota_get_deployment_status() == RPI_CONNECT_OTA_DEPLOYMENT_APPLYING) {
        char *applied_id = rpi_connect_ota_get_deployment_id();
        if (!applied_id) {
            // No deployment id recorded; clear the stale status.
            rpi_connect_ota_set_deployment_status(RPI_CONNECT_OTA_DEPLOYMENT_IDLE);
        } else if (!rpi_connect_ota_boot_is_flash_update()) {
            RPI_CONNECT_OTA_ERROR("Update was not applied for deployment ID=%s; reporting failure\n", applied_id);
            // On failure the state stays APPLYING and is retried next boot.
            rpi_connect_ota_fail_deployment(token, applied_id, "update not applied");
            free(applied_id);
        } else {
            RPI_CONNECT_OTA_INFO("Update applied; reporting success for deployment ID=%s\n", applied_id);
            retries = RPI_CONNECT_OTA_SERVER_RETRIES;
            rc = rpi_connect_ota_complete_deployment(token, applied_id);
            while (retries-- && rc != 0) {
                sleep_ms(RPI_CONNECT_OTA_SERVER_RETRY_DELAY_MS);
                rc = rpi_connect_ota_complete_deployment(token, applied_id);
            }
            free(applied_id);
            if (rc != 0) {
                // Success could not be reported, so skip the "buy" below: the
                // bootrom rolls back on the next reset and the old image then
                // reports the failure (above).
                RPI_CONNECT_OTA_ERROR("Failed to report applied deployment; not committing the update\n");
                return rc;
            }
        }
    }

    rpi_connect_ota_handle_boot();
    return 0;
}

char *rpi_connect_ota_get_resumable_deployment_id(void) {
    rpi_connect_ota_deployment_status_t status = rpi_connect_ota_get_deployment_status();
    // APPLYING means the image is already installed and awaiting its success
    // report (handled by rpi_connect_ota_boot_sync); do not re-download it.
    if (status == RPI_CONNECT_OTA_DEPLOYMENT_APPLYING)
        return NULL;

    // Resume on any stored deployment id, not just status DOWNLOADING: a reboot
    // can land before the status write, and re-downloading is safe.
    char *deployment_id = rpi_connect_ota_get_deployment_id();
    if (!deployment_id && status != RPI_CONNECT_OTA_DEPLOYMENT_IDLE) {
        // No deployment id recorded; clear the stale status.
        rpi_connect_ota_set_deployment_status(RPI_CONNECT_OTA_DEPLOYMENT_IDLE);
    }
    return deployment_id;
}

// Internal state threaded through the async download callbacks
typedef struct {
    rpi_connect_sha256_ctx_t sha256_ctx;
    rpi_connect_ota_data_callback_t user_callback;
    void *user_arg;
    size_t total_bytes;
    int complete;
    int error;
    int user_error;
} ota_dl_ctx_t;

struct rpi_connect_ota_download_ctx {
    ota_dl_ctx_t dl;
    rpi_connect_download_context_t *dl_ctx;
    char *expected_checksum;
    int finished;
};

// Run SHA-256 finish + checksum verification.
// On success returns 0 and fills `result`. On failure returns -1.
static int ota_finalise(rpi_connect_ota_download_ctx_t *ctx,
                        rpi_connect_ota_download_result_t *result) {
    unsigned char hash[RPI_CONNECT_SHA256_SIZE];
    if (rpi_connect_sha256_finish(&ctx->dl.sha256_ctx, hash) != 0) {
        RPI_CONNECT_OTA_ERROR("Failed to finalise SHA256\n");
        return -1;
    }

    char hex[RPI_CONNECT_SHA256_SIZE * 2 + 1];
    for (int i = 0; i < RPI_CONNECT_SHA256_SIZE; i++)
        snprintf(&hex[i * 2], 3, "%02x", hash[i]);

    if (ctx->expected_checksum && strcmp(hex, ctx->expected_checksum) != 0) {
        RPI_CONNECT_OTA_ERROR("Checksum mismatch: expected=%s got=%s\n",
                          ctx->expected_checksum, hex);
        return -1;
    }

    result->total_bytes = ctx->dl.total_bytes;
    memcpy(result->sha256_hex, hex, sizeof(hex));
    return 0;
}

static int ota_data_cb(const void *data, size_t data_len, void *arg) {
    rpi_connect_ota_download_ctx_t *ctx = (rpi_connect_ota_download_ctx_t *)arg;
    if (data == NULL && data_len == 0) {
        RPI_CONNECT_OTA_DEBUG("Download complete: total bytes=%zu\n", ctx->dl.total_bytes);
        ctx->dl.complete = 1;
        return 0;
    }
    ctx->dl.total_bytes += data_len;
    rpi_connect_sha256_update(&ctx->dl.sha256_ctx, data, data_len);
    if (ctx->dl.user_callback) {
        int rc = ctx->dl.user_callback(data, data_len, ctx->dl.user_arg);
        if (rc != 0) {
            ctx->dl.user_error = rc;
            return rc;
        }
    }
    size_t content_len = rpi_connect_download_content_length(ctx->dl_ctx);
    if (content_len > 0) {
        RPI_CONNECT_OTA_DEBUG("Download chunk: %zu bytes (total=%zu of %zu, %u%%)\n",
                          data_len, ctx->dl.total_bytes, content_len,
                          (unsigned)((ctx->dl.total_bytes * 100) / content_len));
    } else {
        RPI_CONNECT_OTA_DEBUG("Download chunk: %zu bytes (total=%zu)\n", data_len, ctx->dl.total_bytes);
    }
    return 0;
}

static void ota_error_cb(int error_code, void *arg) {
    rpi_connect_ota_download_ctx_t *ctx = (rpi_connect_ota_download_ctx_t *)arg;
    RPI_CONNECT_OTA_ERROR("Download error: code=%d\n", error_code);
    ctx->dl.error = error_code;
}

rpi_connect_ota_download_ctx_t *rpi_connect_ota_download_start(
    const char *uri,
    const char *expected_checksum,
    rpi_connect_ota_data_callback_t user_callback,
    void *user_arg,
    bool tls_no_verify) {
    rpi_connect_ota_download_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->dl.user_callback = user_callback;
    ctx->dl.user_arg = user_arg;

    if (rpi_connect_sha256_init(&ctx->dl.sha256_ctx) != 0) {
        RPI_CONNECT_OTA_ERROR("Failed to initialise SHA256 context\n");
        free(ctx);
        return NULL;
    }

    if (expected_checksum) {
        ctx->expected_checksum = strdup(expected_checksum);
        if (!ctx->expected_checksum) {
            free(ctx);
            return NULL;
        }
    }

    ctx->dl_ctx = rpi_connect_async_download(
        rpi_connect_default_async_context(), uri,
        ota_data_cb, ota_error_cb, ctx, tls_no_verify);

    if (!ctx->dl_ctx) {
        RPI_CONNECT_OTA_ERROR("Failed to start async download\n");
        free(ctx->expected_checksum);
        free(ctx);
        return NULL;
    }

    return ctx;
}

int rpi_connect_ota_download_poll(
    rpi_connect_ota_download_ctx_t *ctx,
    rpi_connect_ota_download_result_t *result) {
    if (ctx->finished)
        return ctx->finished;

    rpi_connect_download_poll(ctx->dl_ctx);

    if (!ctx->dl.complete && !ctx->dl.error && !ctx->dl.user_error)
        return 0;

    if (ctx->dl.error || ctx->dl.user_error) {
        if (ctx->dl.error)
            RPI_CONNECT_OTA_ERROR("Download failed with error %d\n", ctx->dl.error);
        ctx->finished = -1;
        return -1;
    }

    RPI_CONNECT_OTA_DEBUG("Download succeeded: %zu bytes\n", ctx->dl.total_bytes);

    rpi_connect_ota_download_result_t local;
    if (ota_finalise(ctx, &local) != 0) {
        ctx->finished = -1;
        return -1;
    }
    if (result)
        *result = local;

    ctx->finished = 1;
    return 1;
}

size_t rpi_connect_ota_download_content_length(rpi_connect_ota_download_ctx_t *ctx) {
    if (!ctx)
        return 0;
    return rpi_connect_download_content_length(ctx->dl_ctx);
}

size_t rpi_connect_ota_download_bytes_so_far(rpi_connect_ota_download_ctx_t *ctx) {
    if (!ctx)
        return 0;
    return ctx->dl.total_bytes;
}

bool rpi_connect_ota_download_error_is_transient(rpi_connect_ota_download_ctx_t *ctx) {
    // Transport-level failures (connection refused, DNS, dropped connection)
    // are reported as a negative error code. An explicit HTTP error response
    // (e.g. 404) is reported as the positive status code, and checksum or
    // data-callback failures leave the error code at 0: neither is transient.
    return ctx && ctx->dl.error < 0;
}

void rpi_connect_ota_download_stop(rpi_connect_ota_download_ctx_t *ctx) {
    if (!ctx)
        return;
    rpi_connect_download_stop(ctx->dl_ctx);
    // Failed/aborted downloads never reach sha256_finish; discard the digest.
    rpi_connect_sha256_abort(&ctx->dl.sha256_ctx);
    free(ctx->expected_checksum);
    free(ctx);
}

int rpi_connect_ota_download(
    const char *uri,
    const char *expected_checksum,
    rpi_connect_ota_data_callback_t user_callback,
    void *user_arg,
    bool tls_no_verify,
    rpi_connect_ota_download_result_t *result) {
    rpi_connect_ota_download_ctx_t *ctx = rpi_connect_ota_download_start(
        uri, expected_checksum, user_callback, user_arg, tls_no_verify);
    if (!ctx)
        return -1;

    int rc;
    while ((rc = rpi_connect_ota_download_poll(ctx, result)) == 0) {
#if PICO_ON_DEVICE
        async_context_poll(rpi_connect_default_async_context());
#endif
        sleep_ms(10);
    }

    rpi_connect_ota_download_stop(ctx);
    return rc > 0 ? 0 : -1;
}

// The client is authoritative for its capabilities: the full known set is
// declared via POST /me, with anything this client does not support (e.g. a
// capability of a previous firmware image) marked unavailable.
int rpi_connect_ota_register_ota_capability(const char *token) {
    return rpi_connect_send_capability(token, RPI_CONNECT_CAPABILITY_OTA);
}

static rpi_connect_ota_deploy_callback_t g_ota_deploy_cb;
static void *g_ota_deploy_cb_arg;

static void rpi_connect_ota_event_callback(const rpi_connect_event_t *event, __unused void *arg) {
    switch (event->type) {
        case RPI_CONNECT_EVENT_TYPE_DEPLOY:
            RPI_CONNECT_OTA_DEBUG("Received DEPLOY ID=%s\n", event->data.deploy.id);
            if (g_ota_deploy_cb)
                g_ota_deploy_cb(event->data.deploy.id, g_ota_deploy_cb_arg);
            break;
        case RPI_CONNECT_EVENT_TYPE_PING:
            RPI_CONNECT_OTA_DEBUG("Received PING event: Timestamp=%llu\n",
                              (unsigned long long)event->data.ping.timestamp);
            break;
        case RPI_CONNECT_EVENT_TYPE_WELCOME:
            RPI_CONNECT_OTA_DEBUG("Received WELCOME event: SID=%s\n",
                              event->data.welcome.sid);
            break;
        case RPI_CONNECT_EVENT_TYPE_CONFIRM_SUBSCRIPTION:
            RPI_CONNECT_OTA_DEBUG("Received CONFIRM_SUBSCRIPTION event: Identifier=%s\n",
                              event->data.confirm_subscription.identifier);
            break;
        case RPI_CONNECT_EVENT_TYPE_UNKNOWN:
            RPI_CONNECT_OTA_DEBUG("Received UNKNOWN event: SID=%s\n",
                              event->data.welcome.sid);
            break;
        default:
            RPI_CONNECT_OTA_DEBUG("Received unknown event type: %d\n", event->type);
            break;
    }
}

rpi_connect_event_context_t *rpi_connect_ota_event_listen(
        async_context_t *context, const char *token,
        rpi_connect_ota_deploy_callback_t deploy_cb, void *deploy_arg) {
    g_ota_deploy_cb = deploy_cb;
    g_ota_deploy_cb_arg = deploy_arg;
    return rpi_connect_event_listen(context, token, rpi_connect_ota_event_callback, NULL);
}

int rpi_connect_ota_device_identity_exchange(
    const char *client_id,
    const char *serial_number,
    const char *hostname,
    const unsigned char *private_key_32,
    const char *public_key_pem,
    char **out_token) {
    if (out_token)
        *out_token = NULL;

    if (!client_id || !serial_number || !hostname || !private_key_32 || !public_key_pem) {
        RPI_CONNECT_OTA_ERROR("device_identity_exchange: missing required argument\n");
        return -1;
    }

    char *device_id = NULL;
    char *token = rpi_connect_device_identity_exchange(
        client_id, private_key_32, public_key_pem, hostname, serial_number, &device_id);

    if (!token) {
        RPI_CONNECT_OTA_ERROR("device_identity_exchange: failed\n");
        free(device_id);
        return -1;
    }

    RPI_CONNECT_OTA_INFO("device_identity_exchange: device_id=%s token=%s\n",
                     device_id ? device_id : "(null)", token);
    free(device_id);

    if (out_token)
        *out_token = token;
    else
        free(token);
    return 0;
}

#if PICO_ON_DEVICE
bool rpi_connect_ota_boot_is_flash_update(void) {
    return rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE;
}

int rpi_connect_ota_handle_boot(void) {
    boot_info_t boot_info = { 0 };

    // This function returns false or true, not a PICO_ERROR_xxx
    if (!rom_get_boot_info(&boot_info)) {
        return PICO_ERROR_NOT_FOUND;
    }
    RPI_CONNECT_OTA_INFO("Boot partition was: %d\n", boot_info.partition);

    if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
        RPI_CONNECT_OTA_INFO("Flash update detected\n");
        if (boot_info.reboot_params[0]) {
            RPI_CONNECT_OTA_INFO("Flash update base was: %x\n", (unsigned)boot_info.reboot_params[0]);
        }

        if (boot_info.tbyb_and_update_info){
            RPI_CONNECT_OTA_INFO("Update info: %x\n", boot_info.tbyb_and_update_info);
        }
        uint8_t *buffer;
        size_t buffer_size;
        int status = rpi_connect_ota_get_workarea(&buffer, &buffer_size);
        if (PICO_OK != status) {
            RPI_CONNECT_OTA_ERROR("Failed to get work area for flash image: %d\n", status);
            return status;
        }

        status = rom_explicit_buy(buffer, buffer_size);
        if (PICO_OK != status) {
            RPI_CONNECT_OTA_ERROR("Buy returned: %d\n", status);
        }
        return 1;
    }

    return PICO_OK;
}

int rpi_connect_ota_try_booting_to_flash_update(void) {
    uint8_t *workarea;
    size_t workarea_size;
    int rc = rpi_connect_ota_get_workarea(&workarea, &workarea_size);
    if (rc != 0)
        return rc;

    bool complete = false;
    uint32_t update_start_addr = 0;
    pico_flash_image_check_write_complete(workarea, &complete, &update_start_addr);
    if (!complete) {
        RPI_CONNECT_OTA_ERROR("Flash update incomplete, cannot reboot\n");
        pico_flash_image_reset_to_idle(workarea);
        return -1;
    }

    RPI_CONNECT_OTA_INFO("Rebooting to flash update at 0x%08x\n", (unsigned)update_start_addr);
    rc = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, 500, update_start_addr, 0);
    if (rc != PICO_OK)
        return rc;

    // rom_reboot schedules the reset 500 ms out and returns immediately. Do
    // not return: the caller treats returning as failure and would race a
    // fail_deployment server report against the pending reboot.
    while (true)
        sleep_ms(100);
}

int rpi_connect_ota_flash_image_data_callback(const void *data, size_t data_len, void *arg) {
    int rc = pico_flash_image_write_data(arg, data, (uint)data_len);
    if (rc != PICO_OK) {
        RPI_CONNECT_OTA_ERROR("Flash write error: %d\n", rc);
        return -1;
    }
    return 0;
}

struct rpi_connect_ota_install_update_ctx {
    rpi_connect_ota_download_ctx_t *ota_ctx;
    uint8_t *workarea;
};

// Common setup: reserve workarea + configure flash-image driver for the update.
// Returns the workarea on success, NULL on failure.
static uint8_t *install_update_prepare_flash(void) {
    uint8_t *workarea;
    size_t workarea_size;
    int rc = rpi_connect_ota_get_workarea(&workarea, &workarea_size);
    if (rc != 0)
        return NULL;

    rc = pico_flash_image_supply_storage(workarea, workarea_size);
    if (rc != PICO_OK) {
        RPI_CONNECT_OTA_ERROR("Failed to supply flash image storage: %d\n", rc);
        return NULL;
    }
    rc = pico_flash_image_config_for_update(workarea, 0);
    if (rc != PICO_OK) {
        RPI_CONNECT_OTA_ERROR("Failed to configure flash image for update: %d\n", rc);
        return NULL;
    }
    return workarea;
}

rpi_connect_ota_install_update_ctx_t *rpi_connect_ota_install_update_start(
    const char *uri, const char *expected_checksum, bool tls_no_verify) {
    if (!expected_checksum || !*expected_checksum) {
        RPI_CONNECT_OTA_ERROR("Install update requires a non-empty checksum\n");
        return NULL;
    }

    uint8_t *workarea = install_update_prepare_flash();
    if (!workarea)
        return NULL;

    rpi_connect_ota_download_ctx_t *ota_ctx = rpi_connect_ota_download_start(
        uri, expected_checksum, rpi_connect_ota_flash_image_data_callback, workarea,
        tls_no_verify);
    if (!ota_ctx)
        return NULL;

    rpi_connect_ota_install_update_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        rpi_connect_ota_download_stop(ota_ctx);
        return NULL;
    }
    ctx->ota_ctx = ota_ctx;
    ctx->workarea = workarea;
    return ctx;
}

int rpi_connect_ota_install_update_poll(rpi_connect_ota_install_update_ctx_t *ctx,
                                    rpi_connect_ota_download_result_t *result) {
    rpi_connect_ota_download_result_t local;
    int rc = rpi_connect_ota_download_poll(ctx->ota_ctx, &local);
    if (rc == 0)
        return 0;

    fimg_update_info_t update_info;
    pico_flash_image_get_op_status(&update_info);
    RPI_CONNECT_OTA_INFO("Update info: family_id=0x%08x, total_blocks=%u, rxed_blocks=%u, rxed_data_index=%u\n",
                      (unsigned)update_info.update_family_id, (unsigned)update_info.total_num_blocks,
                      (unsigned)update_info.rxed_block_count, (unsigned)update_info.rxed_data_index);

    if (rc < 0) {
        RPI_CONNECT_OTA_ERROR("Flash update incomplete after download\n");
        pico_flash_image_reset_to_idle(ctx->workarea);
        return -1;
    }

    RPI_CONNECT_OTA_DEBUG("Download succeeded: %zu bytes, sha256=%s\n", local.total_bytes, local.sha256_hex);
    if (result)
        *result = local;
    return 1;
}

size_t rpi_connect_ota_install_update_bytes_so_far(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx ? rpi_connect_ota_download_bytes_so_far(ctx->ota_ctx) : 0;
}

size_t rpi_connect_ota_install_update_content_length(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx ? rpi_connect_ota_download_content_length(ctx->ota_ctx) : 0;
}

void rpi_connect_ota_install_update_stop(rpi_connect_ota_install_update_ctx_t *ctx) {
    if (!ctx)
        return;
    rpi_connect_ota_download_stop(ctx->ota_ctx);
    free(ctx);
}

bool rpi_connect_ota_install_update_error_is_transient(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx && rpi_connect_ota_download_error_is_transient(ctx->ota_ctx);
}

int rpi_connect_ota_install_update(const char *uri, const char *expected_checksum,
                                   bool tls_no_verify) {
    rpi_connect_ota_install_update_ctx_t *ctx =
        rpi_connect_ota_install_update_start(uri, expected_checksum, tls_no_verify);
    if (!ctx)
        return -1;

    int rc;
    while ((rc = rpi_connect_ota_install_update_poll(ctx, NULL)) == 0) {
        async_context_poll(rpi_connect_default_async_context());
        sleep_ms(10);
    }

    rpi_connect_ota_install_update_stop(ctx);
    return rc > 0 ? 0 : -1;
}

int rpi_connect_ota_read_identity_key_otp(unsigned int start_row, unsigned char out_key[32]) {
    volatile uint16_t *otp_data = (volatile uint16_t *)OTP_DATA_GUARDED_BASE;
    size_t key_offset = 0;
    unsigned int rows = 32 / 2; // 16 rows of 2 bytes

    for (unsigned int i = 0; i < rows && key_offset < 32; i++) {
        uint16_t row = otp_data[start_row + i];
        for (int b = 0; b < 2 && key_offset < 32; b++) {
            out_key[key_offset++] = (unsigned char)(row & 0xff);
            row >>= 8;
        }
    }
    return 0;
}

bool rpi_connect_ota_identity_key_programmed(void) {
    unsigned char key[32];
    rpi_connect_ota_read_identity_key_otp(RPI_CONNECT_IDENTITY_OTP_ROW, key);
    for (size_t i = 0; i < sizeof(key); i++) {
        if (key[i] != 0)
            return true;
    }
    return false;
}

#endif