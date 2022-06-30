/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/sem.h"
#include "pico/cyw43_arch.h"
#include "cyw43_stats.h"

#if PICO_CYW43_ARCH_POLL
#include <lwip/init.h>
#include "lwip/timeouts.h"

#if CYW43_LWIP && !NO_SYS
#error PICO_CYW43_ARCH_POLL requires lwIP NO_SYS=1
#endif

#define CYW43_GPIO_IRQ_HANDLER_PRIORITY 0x40

#ifndef NDEBUG
uint8_t cyw43_core_num;
#endif

bool cyw43_poll_required;

// GPIO interrupt handler to tell us there's cyw43 has work to do
static void gpio_irq_handler(void)
{
    uint32_t events = gpio_get_irq_event_mask(CYW43_PIN_WL_HOST_WAKE);
    if (events & GPIO_IRQ_LEVEL_HIGH) {
        // As we use a high level interrupt, it will go off forever until it's serviced
        // So disable the interrupt until this is done. It's re-enabled again by CYW43_POST_POLL_HOOK
        // which is called at the end of cyw43_poll_func
        gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, false);
        // also clear the force bit which we use to programmatically cause this handler to fire (on the right core)
        io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num() ?
                                          &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;
        hw_clear_bits(&irq_ctrl_base->intf[CYW43_PIN_WL_HOST_WAKE/8], GPIO_IRQ_LEVEL_HIGH << (4 * (CYW43_PIN_WL_HOST_WAKE & 7)));
        cyw43_schedule_internal_poll_dispatch(cyw43_poll);
        CYW43_STAT_INC(IRQ_COUNT);
    }
}

void cyw43_post_poll_hook(void) {
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
}

int cyw43_arch_init(void) {
#ifndef NDEBUG
    cyw43_core_num = (uint8_t)get_core_num();
#endif
    cyw43_init(&cyw43_state);
    static bool done_lwip_init;
    if (!done_lwip_init) {
        lwip_init();
        done_lwip_init = true;
    }
    gpio_add_raw_irq_handler_with_order_priority(IO_IRQ_BANK0, gpio_irq_handler, CYW43_GPIO_IRQ_HANDLER_PRIORITY);
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    return 0;
}

void cyw43_arch_deinit(void) {
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, false);
    gpio_remove_raw_irq_handler(IO_IRQ_BANK0, gpio_irq_handler);
    cyw43_deinit(&cyw43_state);
}


void cyw43_schedule_internal_poll_dispatch(__unused void (*func)(void)) {
    cyw43_poll_required = true;
}

void cyw43_arch_poll(void)
{
    CYW43_STAT_INC(LWIP_RUN_COUNT);
    sys_check_timeouts();
    if (cyw43_poll) {
        if (cyw43_sleep > 0) {
            // todo check this; but we don't want to advance too quickly
            static absolute_time_t last_poll_time;
            absolute_time_t current = get_absolute_time();
            if (absolute_time_diff_us(last_poll_time, current) > 1000) {
                if (--cyw43_sleep == 0) {
                    cyw43_poll_required = 1;
                }
                last_poll_time = current;
            }
        }
        // todo graham i removed this because otherwise polling can do nothing during connect.
        //  in the polling only case, the caller is responsible for throttling how often they call anyway.
        //  The alternative would be to have the call to this function from the init set the poll_required flag first
//        if (cyw43_poll_required) {
            cyw43_poll();
//            cyw43_poll_required = false;
//        }
    }
}

#ifndef NDEBUG
void cyw43_thread_check() {
    if (__get_current_exception() || get_core_num() != cyw43_core_num) {
        panic("cyw43_thread_lock_check failed");
    }
}
#endif

#endif