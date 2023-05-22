/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PICO_HTTP_CLIENT_UTIL_H
#define PICO_HTTP_CLIENT_UTIL_H

#include "lwip/apps/http_client.h"

/*! \brief Parameters used to make HTTP request
 *  \ingroup pico_lwip
 */
typedef struct PICO_HTTP_REQUEST {
    /*!
     * The name of the host, e.g. www.raspberrypi.com
     */
    const char *hostname;
    /*!
     * The url to request, e.g. /favicon.ico
     */
    const char *url;
    /*!
     * Function to callback with headers, can be null
     * @see httpc_headers_done_fn
     */
    httpc_headers_done_fn headers_fn;
    /*!
     * Function to callback with results from the server, can be null
     * @see altcp_recv_fn
     */
    altcp_recv_fn recv_fn;
    /*!
     * Function to callback with final results of the request, can be null
     * @see httpc_result_fn
     */
    httpc_result_fn result_fn;
    /*!
     * Callback to pass to calback functions
     */
    void *callback_arg;
    /*!
     * The port to use. A default port is chosen if this is set to zero
     */
    uint16_t port;
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
#endif
    /*!
     * LwIP HTTP client settings
     */
    httpc_connection_t settings;
    /*!
     * Flag to indicate when the request is complete
     */
    int complete;
    /*!
     * Overall result of http request, only valid when complete is set
     */
    httpc_result_t result;

} PICO_HTTP_REQUEST_T;

struct async_context;

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
int http_client_request_async(struct async_context *context, PICO_HTTP_REQUEST_T *req);

/*! \brief Perform a http request synchronously
 *  \ingroup pico_lwip
 *
 * Perform the http request synchronously
 *
 * @param context async context
 * @param req HTTP request parameters. As a minimum this should be initialised to zero with hostname and url set to valid values
 * @param result Returns the overall result of the http request when complete. Zero indicates success.
 */
int http_client_request_sync(struct async_context *context, PICO_HTTP_REQUEST_T *req);

/*! \brief A http header callback that can be passed to \em http_client_init or \em http_client_init_secure
 *  \ingroup pico_http_client
 *
 * An implementation of the http header callback which just prints headers to stdout
 *
 * @param arg argument specified on initialisation
 * @param hdr header pbuf(s) (may contain data also)
 * @param hdr_len length of the headers in 'hdr'
 * @param content_len content length as received in the headers (-1 if not received)
 * @return if != zero is returned, the connection is aborted
 */
err_t http_client_header_print_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

/*! \brief A http recv callback that can be passed to http_client_init or http_client_init_secure
 *  \ingroup pico_http_client
 *
 * An implementation of the http recv callback which just prints the http body to stdout
 *
 * @param arg argument specified on initialisation
 * @param conn http client connection
 * @param p body pbuf(s)
 * @param err Error code in the case of an error
 * @return if != zero is returned, the connection is aborted
 */
err_t http_client_receive_print_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);

#endif
