/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PIO_ENUMS_H
#define _PIO_ENUMS_H

typedef unsigned int uint;

enum struct fifo_config {
    txrx = 0,
    tx = 1,
    rx = 2,
    txget = 3,
    txput = 4,
    putget = 5,
};

#endif