/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/asm_helper.S"

// This has to happen after memcpy() and memset() are available.
__pre_init tls_init, 10000
