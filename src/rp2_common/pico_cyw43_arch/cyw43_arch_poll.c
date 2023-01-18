/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/cyw43_arch.h"
#include "pico/cyw43_driver.h"

#if PICO_CYW43_ARCH_POLL
#include "pico/async_context_poll.h"
#if CYW43_LWIP
#include "pico/lwip_nosys.h"
#endif

#if CYW43_LWIP && !NO_SYS
#error PICO_CYW43_ARCH_POLL requires lwIP NO_SYS=1
#endif

static async_context_poll_t cyw43_async_context_poll;

async_context_t *cyw43_arch_init_default_async_context(void) {
    if (async_context_poll_init_with_defaults(&cyw43_async_context_poll))
        return &cyw43_async_context_poll.core;
    return NULL;
}

int cyw43_arch_init(void) {
    async_context_t *context = cyw43_arch_async_context();
    if (!context) {
        context = cyw43_arch_init_default_async_context();
        if (!context) return PICO_ERROR_GENERIC;
        cyw43_arch_set_async_context(context);
    }
    bool ok = cyw43_driver_init(context);
#if CYW43_LWIP
    ok &= lwip_nosys_init(context);
#endif
    if (!ok) {
        cyw43_arch_deinit();
        return PICO_ERROR_GENERIC;
    } else {
        return 0;
    }
}

void cyw43_arch_deinit(void) {
    async_context_t *context = cyw43_arch_async_context();
#if CYW43_LWIP
    lwip_nosys_deinit(context);
#endif
    cyw43_driver_deinit(context);
    // if it is our context, then we de-init it.
    if (context == &cyw43_async_context_poll.core) {
        async_context_deinit(context);
        cyw43_arch_set_async_context(NULL);
    }
}

#endif
