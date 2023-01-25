/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EXAMPLE_CYW43_ARCH_ARCH_FREERTOS_SYS_H
#define _EXAMPLE_CYW43_ARCH_ARCH_FREERTOS_SYS_H

#include "pico/cyw43_arch/arch_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void cyw43_thread_enter(void);
void cyw43_thread_exit(void);

#define CYW43_THREAD_ENTER cyw43_thread_enter();
#define CYW43_THREAD_EXIT cyw43_thread_exit();
#ifndef NDEBUG
void cyw43_thread_lock_check(void);
#define cyw43_arch_lwip_check() cyw43_thread_lock_check()
#define CYW43_THREAD_LOCK_CHECK cyw43_arch_lwip_check();
#else
#define cyw43_arch_lwip_check() ((void)0)
#define CYW43_THREAD_LOCK_CHECK
#endif

void cyw43_await_background_or_timeout_us(uint32_t timeout_us);
// todo not 100% sure about the timeouts here; MP uses __WFI which will always wakeup periodically
#define CYW43_SDPCM_SEND_COMMON_WAIT cyw43_await_background_or_timeout_us(1000);
#define CYW43_DO_IOCTL_WAIT cyw43_await_background_or_timeout_us(1000);

void cyw43_delay_ms(uint32_t ms);
void cyw43_delay_us(uint32_t us);

void cyw43_schedule_internal_poll_dispatch(void (*func)(void));

void cyw43_post_poll_hook(void);
#define CYW43_POST_POLL_HOOK cyw43_post_poll_hook();

static inline void cyw43_arch_lwip_begin(void) {
    cyw43_thread_enter();
}
static inline void cyw43_arch_lwip_end(void) {
    cyw43_thread_exit();
}

static inline int cyw43_arch_lwip_protect(int (*func)(void *param), void *param) {
    cyw43_arch_lwip_begin();
    int rc = func(param);
    cyw43_arch_lwip_end();
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif