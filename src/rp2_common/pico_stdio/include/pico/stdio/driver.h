/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_STDIO_DRIVER_H
#define _PICO_STDIO_DRIVER_H

#include "pico/stdio.h"

/*! \brief A stdio driver that can be registered with the pico_stdio subsystem
 *  \ingroup pico_stdio
 *
 * Drivers are linked into a list and called in turn for each I/O operation.
 * Implement only the callbacks relevant to your driver; unused callbacks may
 * be left as NULL.
 */
struct stdio_driver {
    void (*out_chars)(const char *buf, int len); ///< Output characters to the driver; may be NULL if output is not supported
    void (*out_flush)(void); ///< Flush any buffered output; may be NULL
    int (*in_chars)(char *buf, int len); ///< Read available input characters; returns the number read, or may be NULL if input is not supported
    void (*set_chars_available_callback)(void (*fn)(void*), void *param); ///< Register a callback to be invoked when input characters become available; may be NULL
    stdio_driver_t *next; ///< Next driver in the linked list; managed by the stdio subsystem
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    bool last_ended_with_cr; ///< Private implementation detail
    bool crlf_enabled; ///< Whether CR/LF translation is enabled for this driver
#endif
};

#endif
