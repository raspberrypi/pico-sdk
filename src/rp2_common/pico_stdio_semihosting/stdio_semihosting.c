/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include "pico/stdio/driver.h"
#include "pico/stdio_semihosting.h"
#include "pico/binary_info.h"

static void __attribute__((naked)) semihosting_puts(__unused const char *s) {
   __asm (

   "mov r1, r0\n"
   "mov r0, #4\n"
   "bkpt 0xab\n"
   "bx lr\n"
   );
}

static void stdio_semihosting_out_chars(const char *buf, int length) {
    // Add \n and zero terminate the buffer
    char *msg = (char *) malloc(length + 2);
    if (msg != NULL) {
        memcpy(msg, buf, length);
        msg[length] = '\n';
        msg[length + 1] = 0;
        semihosting_puts(msg);
        free(msg);
    }
}

stdio_driver_t stdio_semihosting = {
        .out_chars = stdio_semihosting_out_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
        .crlf_enabled = PICO_STDIO_SEMIHOSTING_DEFAULT_CRLF
#endif
};

void stdio_semihosting_init() {
#if !PICO_NO_BI_STDIO_SEMIHOSTING
    bi_decl_if_func_used(bi_program_feature("semihosting stdout"));
#endif
    stdio_set_driver_enabled(&stdio_semihosting, true);
}

