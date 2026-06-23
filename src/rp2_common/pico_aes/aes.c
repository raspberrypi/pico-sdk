/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/aes.h"
#include "pico/bootrom.h"
#include "hardware/structs/otp.h"
#include "hardware/rcp.h"

#if !PICO_AES_CONFIGURED
#error "pico_aes linker script is not configured - have you called the pico_aes_configure_binary CMake function on your target"
#endif


// From aes.S
extern void pico_aes_decrypt_internal(uint8_t* key4way, uint8_t* IV_OTPsalt, uint8_t* IV_public, uint8_t(*buf)[16], int nblk);

// State
typedef struct pico_aes_context {
    uint32_t otp_key_page;
} pico_aes_context_t;


static pico_aes_context_t aes_context = {};


// Stop the compiler from constant-folding a hardware base pointer into the
// pointers to individual registers, in cases where constant folding has
// produced redundant 32-bit pointer literals that could have been load/store
// offsets. (Note typeof(ptr+0) gives non-const, for +r constraint.) E.g.
//     uart_hw_t *uart0 = __get_opaque_ptr(uart0_hw);
#define __get_opaque_ptr(ptr) ({ \
    typeof((ptr)+0) __opaque_ptr = (ptr); \
    asm ("" : "+r"(__opaque_ptr)); \
    __opaque_ptr; \
})


// The function lock_key() is called from pico_aes_decrypt_internal() after key initialisation is complete and before decryption begins.
// That is a suitable point to lock the OTP area where key information is stored.
__weak void pico_aes_lock_key() {
    io_rw_32 *sw_lock = __get_opaque_ptr(&otp_hw->sw_lock[0]) + aes_context.otp_key_page;
#if PICO_AES_HARDENING
    // prevent compiler from re-using sw_lock pointer
    io_rw_32 *sw_lock2;
    pico_default_asm(
        "ldr %0, =%1\n"
        : "=r" (sw_lock2)
        : "i" (OTP_BASE + OTP_SW_LOCK0_OFFSET)
    );
    sw_lock2 += aes_context.otp_key_page;
    sw_lock[0] = 0xf;
    sw_lock[1] = 0xf;
    uint32_t v = sw_lock2[1];
    v = (v << 4) | sw_lock2[0];
    uint32_t ff1;
    pico_default_asm_volatile(
        "movs %0, #0xff"
        : "=r" (ff1)
    );
    uint32_t ff2;
    pico_default_asm_volatile(
        "movw %0, #0xff"
        : "=r" (ff2)
    );
    rcp_iequal(v, ff1);
#if RC_COUNT
    rcp_count_check_nodelay(31 + PICO_AES256_RCP_COUNT_DELTA);
#endif
    rcp_iequal(ff2, v);
#else
    sw_lock[0] = 0xf;
    sw_lock[1] = 0xf;
#if RC_COUNT
    rcp_count_check_nodelay(31 + PICO_AES256_RCP_COUNT_DELTA);
#endif
#endif
}

__weak void pico_aes_lock_all(void) {
    io_rw_32 *sw_lock = __get_opaque_ptr(&otp_hw->sw_lock[0]) + aes_context.otp_key_page;
#if PICO_AES_HARDENING
    // we only actually need to lock page 2 but we lock and check 0, 1, 2 anyway
    // prevent compiler from re-using sw_lock pointer
    io_rw_32 *sw_lock2;
    pico_default_asm(
        "ldr %0, =%1\n"
        : "=r" (sw_lock2)
        : "i" (OTP_BASE + OTP_SW_LOCK0_OFFSET)
    );
    sw_lock2 += aes_context.otp_key_page;
    sw_lock[0] = 0xf;
    sw_lock[1] = 0xf;
    uint32_t v = sw_lock2[1];
    sw_lock[2] = 0xf;
    v = (v << 4) | sw_lock2[0];
    v = (v << 4) | sw_lock2[2];
    uint32_t fff1;
    pico_default_asm_volatile(
        "movw %0, #0xfff"
        : "=r" (fff1)
    );
    uint32_t fff2;
    pico_default_asm_volatile(
        "movw %0, #0xfff"
        : "=r" (fff2)
    );
    rcp_iequal(v, fff1);
    rcp_iequal(fff2, v);
#else
    sw_lock[0] = 0xf;
    sw_lock[1] = 0xf;
    sw_lock[2] = 0xf;
#endif
}

#include <stdio.h>
int pico_aes_try_decrypt(uint8_t(*data)[16], size_t data_size, uint32_t otp_key_page, uint8_t* iv_public) {
    if (!bootrom_try_acquire_lock(BOOTROM_LOCK_SHA_256)) return PICO_ERROR_RESOURCE_IN_USE;

    aes_context.otp_key_page = otp_key_page;

    uint16_t* otp_data = (uint16_t*)OTP_DATA_GUARDED_BASE;

    // Restore aes_scratch_y from stored location in RAM_STORE
    extern uint32_t __aes_scratch_y_source__;
    extern uint32_t __aes_scratch_y_start__;
    extern uint32_t __aes_scratch_y_end__;

    uint32_t stored_words = (uint32_t)(&__aes_scratch_y_end__ - &__aes_scratch_y_start__);
    memcpy(&__aes_scratch_y_start__, &__aes_scratch_y_source__, stored_words * sizeof(uint32_t));

    pico_aes_decrypt_internal(
        (uint8_t*)&(otp_data[otp_key_page * 0x40]),
        (uint8_t*)&(otp_data[(otp_key_page + 2) * 0x40]),
        (uint8_t*)iv_public,
        data,
        data_size/16
    );

    bootrom_release_lock(BOOTROM_LOCK_SHA_256);

    return PICO_OK;
}