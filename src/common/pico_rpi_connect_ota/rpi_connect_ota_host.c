/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Host stand-ins for the bootrom / pico_flash_image parts of the OTA API.
// The downloaded image is written to download.img in the current directory
// and "rebooting" into it is exit(0): one process invocation models one boot.

#include "pico/rpi_connect_ota.h"
#include "pico/rpi_connect.h"
#include "pico/rpi_connect_util.h"

int rpi_connect_ota_handle_boot(void) {
    return 0;
}

bool rpi_connect_ota_boot_is_flash_update(void) {
    // No bootrom on the host: a process restart stands in for booting the new
    // image. Test hook: set RPI_CONNECT_TEST_BOOT_IS_UPDATE=0 to simulate a
    // boot where the update was not applied (bootrom rollback).
    const char *override = getenv("RPI_CONNECT_TEST_BOOT_IS_UPDATE");
    if (override && override[0] == '0') {
        return false;
    }
    return true;
}

int rpi_connect_ota_try_booting_to_flash_update(void) {
    RPI_CONNECT_OTA_INFO("Please check download.img is the expected version.\nExiting\n");
    exit(0);
    return 0;
}

int rpi_connect_ota_flash_image_data_callback(__unused const void *data, __unused size_t data_len, __unused void *arg) {
    return -1;
}

static int file_write_callback(const void *data, size_t data_len, void *arg) {
    FILE *f = (FILE *)arg;
    if (fwrite(data, 1, data_len, f) != data_len) {
        RPI_CONNECT_ERROR("Failed to write download data to file\n");
        return -1;
    }
    return 0;
}

struct rpi_connect_ota_install_update_ctx {
    rpi_connect_ota_download_ctx_t *ota_ctx;
    FILE *file;
};

rpi_connect_ota_install_update_ctx_t *rpi_connect_ota_install_update_start(
    const char *uri, const char *expected_checksum, bool tls_no_verify) {
    if (!expected_checksum || !*expected_checksum) {
        RPI_CONNECT_ERROR("Install update requires a non-empty checksum\n");
        return NULL;
    }

    FILE *f = fopen("download.img", "wb");
    if (!f) {
        RPI_CONNECT_ERROR("Failed to open download.img for writing\n");
        return NULL;
    }

    rpi_connect_ota_download_ctx_t *ota_ctx = rpi_connect_ota_download_start(
        uri, expected_checksum, file_write_callback, f, tls_no_verify);
    if (!ota_ctx) {
        fclose(f);
        return NULL;
    }

    rpi_connect_ota_install_update_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        rpi_connect_ota_download_stop(ota_ctx);
        fclose(f);
        return NULL;
    }
    ctx->ota_ctx = ota_ctx;
    ctx->file = f;
    return ctx;
}

int rpi_connect_ota_install_update_poll(rpi_connect_ota_install_update_ctx_t *ctx,
                                    rpi_connect_ota_download_result_t *result) {
    rpi_connect_ota_download_result_t local;
    int rc = rpi_connect_ota_download_poll(ctx->ota_ctx, &local);
    if (rc == 0) {
        return 0;
    }

    if (rc > 0) {
        RPI_CONNECT_DEBUG("Download succeeded: %zu bytes, sha256=%s\n", local.total_bytes, local.sha256_hex);
        if (result) {
            *result = local;
        }
    }

    return rc;
}

size_t rpi_connect_ota_install_update_bytes_so_far(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx ? rpi_connect_ota_download_bytes_so_far(ctx->ota_ctx) : 0;
}

size_t rpi_connect_ota_install_update_content_length(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx ? rpi_connect_ota_download_content_length(ctx->ota_ctx) : 0;
}

void rpi_connect_ota_install_update_stop(rpi_connect_ota_install_update_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    rpi_connect_ota_download_stop(ctx->ota_ctx);
    if (ctx->file) {
        fclose(ctx->file);
    }
    free(ctx);
}

bool rpi_connect_ota_install_update_error_is_transient(rpi_connect_ota_install_update_ctx_t *ctx) {
    return ctx && rpi_connect_ota_download_error_is_transient(ctx->ota_ctx);
}

int rpi_connect_ota_install_update(const char *uri, const char *expected_checksum,
                                   bool tls_no_verify) {
    rpi_connect_ota_install_update_ctx_t *ctx =
        rpi_connect_ota_install_update_start(uri, expected_checksum, tls_no_verify);
    if (!ctx) {
        return -1;
    }

    int rc;
    while ((rc = rpi_connect_ota_install_update_poll(ctx, NULL)) == 0) {
        sleep_ms(10);
    }

    rpi_connect_ota_install_update_stop(ctx);
    return rc > 0 ? 0 : -1;
}
