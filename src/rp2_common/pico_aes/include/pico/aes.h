/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PICO_AES_H
#define PICO_AES_H

#include "pico.h"
#include "pico/aes/config.h"

#ifndef PICO_AES256_RCP_COUNT_DELTA
#define PICO_AES256_RCP_COUNT_DELTA 0
#endif

#ifndef PICO_AES_CALLER_INIT_RCP_COUNT
#define PICO_AES_CALLER_INIT_RCP_COUNT 0
#endif

#ifndef PICO_AES_INLINE_REF_ROUNDKEY_SHARES_S
#define PICO_AES_INLINE_REF_ROUNDKEY_SHARES_S 1
#endif
#ifndef PICO_AES_INLINE_REF_ROUNDKEY_HVPERMS_S
#define PICO_AES_INLINE_REF_ROUNDKEY_HVPERMS_S 1
#endif
#ifndef PICO_AES_INLINE_SHIFT_ROWS_S
#define PICO_AES_INLINE_SHIFT_ROWS_S 1
#endif
#ifndef PICO_AES_INLINE_MAP_SBOX_S
#define PICO_AES_INLINE_MAP_SBOX_S 1
#endif

#ifndef PICO_AES_HARDENING
#define PICO_AES_HARDENING 1
#endif
#ifndef PICO_AES_DOUBLE_HARDENING
#define PICO_AES_DOUBLE_HARDENING 1
#endif

#ifndef PICO_AES_FIB_WORKAROUND
#define PICO_AES_FIB_WORKAROUND 1
#endif

#ifndef __ASSEMBLER__

int pico_aes_try_decrypt(uint8_t(*data)[16], size_t data_size, uint32_t otp_key_page, uint8_t* iv_public);

void pico_aes_lock_all(void);

#endif

#endif
