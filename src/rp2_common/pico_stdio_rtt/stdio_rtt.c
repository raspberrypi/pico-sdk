/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdio/driver.h"
#include "pico/stdio_rtt.h"
#include "SEGGER_RTT.h"

void stdio_rtt_init(void) {
    stdio_set_driver_enabled(&stdio_rtt, true);
}

static void stdio_rtt_out_chars(const char *buf, int length) {
    SEGGER_RTT_Write(0, buf, length);
}

static int stdio_rtt_in_chars(char *buf, int length) {
    return SEGGER_RTT_Read(0, buf, length);
}

stdio_driver_t stdio_rtt = {
    .out_chars = stdio_rtt_out_chars,
    .in_chars = stdio_rtt_in_chars,
};
