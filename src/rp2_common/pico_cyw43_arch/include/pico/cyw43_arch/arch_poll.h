/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_CYW43_ARCH_ARCH_POLL_H
#define _PICO_CYW43_ARCH_ARCH_POLL_H

#include "pico/cyw43_arch/arch_common.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYW43_THREAD_ENTER
#define CYW43_THREAD_EXIT
#ifndef NDEBUG

void cyw43_thread_check(void);

#define cyw43_arch_lwip_check() cyw43_thread_check()
#define CYW43_THREAD_LOCK_CHECK cyw43_arch_lwip_check();
#else
#define cyw43_arch_lwip_check() ((void)0)
#define CYW43_THREAD_LOCK_CHECK
#endif

#define CYW43_SDPCM_SEND_COMMON_WAIT cyw43_poll_required = true;
#define CYW43_DO_IOCTL_WAIT cyw43_poll_required = true;

#define cyw43_delay_ms sleep_ms
#define cyw43_delay_us sleep_us

void cyw43_schedule_internal_poll_dispatch(void (*func)(void));

void cyw43_post_poll_hook(void);

extern bool cyw43_poll_required;

#define CYW43_POST_POLL_HOOK cyw43_post_poll_hook();
#endif

#ifndef DOXYGEN_GENERATION // multiple definitions in separate headers seems to confused doxygen
#define cyw43_arch_lwip_begin() ((void)0)
#define cyw43_arch_lwip_end() ((void)0)

static inline int cyw43_arch_lwip_protect(int (*func)(void *param), void *param) {
    return func(param);
}

#ifdef __cplusplus
}
#endif

#endif
