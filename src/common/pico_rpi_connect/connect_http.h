/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONNECT_HTTP_H
#define _CONNECT_HTTP_H

#include "lwip/apps/http_client.h"
#include "pico/async_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Sentinel for an unknown response Content-Length
 *
 * Stored in connect_http_request_t::content_len when the server did not send a
 * Content-Length header (e.g. chunked transfer encoding) or the response
 * headers have not yet been received.
 */
#define CONNECT_HTTP_CONTENT_LEN_UNKNOWN 0xFFFFFFFFu

/*! \brief Parameters used to make HTTP request
 *  \ingroup pico_lwip
 */
typedef struct CONNECT_HTTP_REQUEST {
    /*!
     * Function to callback with results from the server, can be null
     */
    void (*key_fn)(const char*, const char*);
    /*!
     * Function to callback with final results of the request, can be null
     */
    void (*result_fn)(struct CONNECT_HTTP_REQUEST *req, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);
    void *result_fn_arg;
    /*!
     * Function to callback with received data, can be null
     */
    size_t (*recv_fn)(void *arg, const char *buf, size_t len);
    void *recv_fn_arg;
    /*!
     * Function to callback with the response headers once they have been
     * received in full, can be null
     */
    void (*headers_fn)(struct CONNECT_HTTP_REQUEST *req, struct pbuf *hdr, u16_t hdr_len);
    /*!
     * Function to callback with extra headers for the request, can be null
     */
    const char *(*extra_headers_fn)(void *arg);
    void *extra_headers_arg;
    /*!
     * Callback to pass to calback functions
     */
    void *callback_arg;
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    /*!
     * TLS configuration, can be null or set to a correctly configured tls configuration.
     * e.g altcp_tls_create_config_client(NULL, 0) would use https without a certificate
     */
    struct altcp_tls_config *tls_config;
    /*!
     * TLS allocator, used internall for setting TLS server name indication
     */
    altcp_allocator_t tls_allocator;
    /*!
     * TLS PCB, used for checking certificate verification result
     */
    struct altcp_pcb *tls_pcb;
    /*!
     * CA certificate tls_config was created with, or NULL if this request
     * does not verify the server certificate. Snapshot taken at request
     * creation so a concurrent change of the process-wide CA (e.g. for an
     * OTA download) cannot affect the verification of this request.
     */
    const char *ca_cert;
    /*!
     * Persistent TLS session for resumption across requests. If non-NULL,
     * the session is restored before connect (when populated) and saved
     * after a successful response. NULL disables resumption.
     */
    struct altcp_tls_session *tls_session;
#endif
    /*!
     * LwIP HTTP client settings
     */
    httpc_connection_t settings;
    /*!
     * Flag to indicate when the request is complete. Written from the lwIP
     * callbacks, which execute as async context work; read it while holding
     * async_context_acquire_lock_blocking() so the read is ordered against
     * the callback's writes on every async_context type (including
     * threadsafe_background, where the callbacks run in an IRQ and may be on
     * the other core).
     */
    int complete;
    /*!
     * Overall result of http request, only valid when complete is set
     */
    httpc_result_t result;

    /*!
     * HTTP state for callbacks
     */
    httpc_state_t *http_state;

    /*!
     * Async context
     */
    async_context_t *context;

    /*!
     * Time request was made
     */
    absolute_time_t before_request;

    /*!
    * HTTP server result from the request or -1 on error
    */
    int http_code;

    /*!
     * Content-Length advertised in the response headers, or
     * CONNECT_HTTP_CONTENT_LEN_UNKNOWN if the server didn't send one or the
     * headers haven't arrived yet. Set by the headers-done callback.
     */
    u32_t content_len;

} connect_http_request_t;

/*! \brief Perform a http request asynchronously
 *  \ingroup pico_lwip
 *
 * Perform the http request asynchronously
 *
 * @param context async context
 * @param req HTTP request parameters. As a minimum this should be initialised to zero with hostname and url set to valid values
 * @return If zero is returned the request has been made and is complete when \em req->complete is true or the result callback has been called.
 *  A non-zero return value indicates an error.
 *
 * @see async_context
 */
int rpi_connect_http_request_async(async_context_t *context, connect_http_request_t *req, const char *uri, u16_t data_len, const char *data);

/*! \brief Set the hostname to use for http requests
 *  \ingroup pico_lwip
 *
 * This must be called before making any requests.
 * This is used for TLS server name indication and for the HTTP host header.
 *
 * @param hostname Hostname of the server to connect to
 */
void rpi_connect_http_set_hostname(const char *hostname);

#ifdef __cplusplus
}
#endif

#endif // _CONNECT_HTTP_H
