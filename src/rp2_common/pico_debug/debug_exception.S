/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/asm_helper.S"

regular_func debug_unhandled_exception
    // On entry, the exception machinery has already pushed R0-R3, R12, LR, return address (PC) and XPSR on either the process or main stack.
    // All the other registers are saved on the main stack here following the layout of the sw_saved_regs_t struct so pico_print_exception()
    // can read their values later.

    // Save registers
    mrs r1, primask
    mrs r2, psp
    movs r3, sp      // always MSP
    push {lr}
    push {r1-r7}     // pushes 7 words
    movs r2, r8
    movs r3, r9
    movs r4, r10
    movs r5, r11
    push {r2-r5}     // pushes 4 words

    // debug_print_exception(exc_return, ipsr, sw_saved_regs)
    // R0 holds the EXC_RETURN, which identifies which stack R0-R3, etc were saved on.
    mrs r1, ipsr
    mov r2, sp
    bl debug_print_exception

    add sp, sp, #(4+7)*4
    pop {pc}
