/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// todo graham #ifdef for LWIP inclusion?

#include "pico/async_context.h"
#include "pico/time.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"

#include "FreeRTOS.h"
#include "semphr.h"

#if NO_SYS
#error lwip_freertos_async_context_bindings requires NO_SYS=0
#endif

static void tcpip_init_done(void *param) {
    xSemaphoreGive((SemaphoreHandle_t)param);
}

bool lwip_freertos_init(async_context_t *context) {
    static bool done_lwip_init;
    if (!done_lwip_init) {
        done_lwip_init = true;
        SemaphoreHandle_t init_sem = xSemaphoreCreateBinary();
        tcpip_init(tcpip_init_done, init_sem);
        xSemaphoreTake(init_sem, portMAX_DELAY);
        vSemaphoreDelete(init_sem);
    }
    return true;
}

void lwip_freertos_deinit(async_context_t *context) {
}


