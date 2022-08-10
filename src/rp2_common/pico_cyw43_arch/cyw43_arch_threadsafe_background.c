/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/mutex.h"
#include "pico/sem.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "cyw43_stats.h"

#if CYW43_LWIP
#include <lwip/init.h>
#include "lwip/timeouts.h"
#endif

// note same code
#if PICO_CYW43_ARCH_THREADSAFE_BACKGROUND

#if PICO_CYW43_ARCH_THREADSAFE_BACKGROUND && CYW43_LWIP && !NO_SYS
#error PICO_CYW43_ARCH_THREADSAFE_BACKGROUND requires lwIP NO_SYS=1
#endif
#if PICO_CYW43_ARCH_THREADSAFE_BACKGROUND && CYW43_LWIP && MEM_LIBC_MALLOC
#error MEM_LIBC_MALLOC is incompatible with PICO_CYW43_ARCH_THREADSAFE_BACKGROUND
#endif
// todo right now we are now always doing a cyw43_dispatch along with a lwip one when hopping cores in low_prio_irq_schedule_dispatch

#ifndef CYW43_SLEEP_CHECK_MS
#define CYW43_SLEEP_CHECK_MS 50 // How often to run lwip callback
#endif
static alarm_id_t periodic_alarm = -1;

static inline uint recursive_mutex_enter_count(recursive_mutex_t *mutex) {
    return mutex->enter_count;
}

static inline lock_owner_id_t recursive_mutex_owner(recursive_mutex_t *mutex) {
    return mutex->owner;
}

#define CYW43_GPIO_IRQ_HANDLER_PRIORITY 0x40

enum {
    CYW43_DISPATCH_SLOT_CYW43 = 0,
    CYW43_DISPATCH_SLOT_ADAPTER,
    CYW43_DISPATCH_SLOT_ENUM_COUNT
};
#ifndef CYW43_DISPATCH_SLOT_COUNT
#define CYW43_DISPATCH_SLOT_COUNT CYW43_DISPATCH_SLOT_ENUM_COUNT
#endif

typedef void (*low_prio_irq_dispatch_t)(void);
static void low_prio_irq_schedule_dispatch(size_t slot, low_prio_irq_dispatch_t f);

static uint8_t cyw43_core_num;
#ifndef NDEBUG
static bool in_low_priority_irq;
#endif
static uint8_t low_priority_irq_num;
static bool low_priority_irq_missed;
static low_prio_irq_dispatch_t low_priority_irq_dispatch_slots[CYW43_DISPATCH_SLOT_COUNT];
static recursive_mutex_t cyw43_mutex;
semaphore_t cyw43_irq_sem;

// Called in low priority pendsv interrupt only to do lwip processing and check cyw43 sleep
static void periodic_worker(void)
{
#if CYW43_USE_STATS
    static uint32_t counter;
    if (counter++ % (30000 / LWIP_SYS_CHECK_MS) == 0) {
        cyw43_dump_stats();
    }
#endif

    CYW43_STAT_INC(LWIP_RUN_COUNT);
#if CYW43_LWIP
    sys_check_timeouts();
#endif
    if (cyw43_poll) {
        if (cyw43_sleep > 0) {
            if (--cyw43_sleep == 0) {
                low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_CYW43, cyw43_poll);
            }
        }
    }
}

// Regular callback to get lwip to check for timeouts
static int64_t periodic_alarm_handler(__unused alarm_id_t id, __unused void *user_data)
{
    // Do lwip processing in low priority pendsv interrupt
    low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_ADAPTER, periodic_worker);
    return CYW43_SLEEP_CHECK_MS * 1000;
}

void cyw43_await_background_or_timeout_us(uint32_t timeout_us) {
    // if we are called from within an IRQ, then don't wait (we are only ever called in a polling loop)
    if (!__get_current_exception()) {
        sem_acquire_timeout_us(&cyw43_irq_sem, timeout_us);
    }
}

// GPIO interrupt handler to tell us there's cyw43 has work to do
static void gpio_irq_handler(void)
{
    uint32_t events = gpio_get_irq_event_mask(CYW43_PIN_WL_HOST_WAKE);
    if (events & GPIO_IRQ_LEVEL_HIGH) {
        // As we use a high level interrupt, it will go off forever until it's serviced
        // So disable the interrupt until this is done. It's re-enabled again by CYW43_POST_POLL_HOOK
        // which is called at the end of cyw43_poll_func
        gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, false);
        // also clear the force bit which we use to progratically cause this handler to fire (on the right core)
        io_irq_ctrl_hw_t *irq_ctrl_base = get_core_num() ?
                                          &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;
        hw_clear_bits(&irq_ctrl_base->intf[CYW43_PIN_WL_HOST_WAKE/8], GPIO_IRQ_LEVEL_HIGH << (4 * (CYW43_PIN_WL_HOST_WAKE & 7)));
        low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_CYW43, cyw43_poll);
        CYW43_STAT_INC(IRQ_COUNT);
    }
}

// Low priority interrupt handler to perform background processing
static void low_priority_irq_handler(void) {
    assert(cyw43_core_num == get_core_num());
    if (recursive_mutex_try_enter(&cyw43_mutex, NULL)) {
        if (recursive_mutex_enter_count(&cyw43_mutex) != 1) {
            low_priority_irq_missed = true;
            CYW43_STAT_INC(PENDSV_DISABLED_COUNT);
        } else {
            CYW43_STAT_INC(PENDSV_RUN_COUNT);
#ifndef NDEBUG
            in_low_priority_irq = true;
#endif
            for (size_t i = 0; i < count_of(low_priority_irq_dispatch_slots); i++) {
                if (low_priority_irq_dispatch_slots[i] != NULL) {
                    low_prio_irq_dispatch_t f = low_priority_irq_dispatch_slots[i];
                    low_priority_irq_dispatch_slots[i] = NULL;
                    f();
                }
            }
#ifndef NDEBUG
            in_low_priority_irq = false;
#endif
        }
        recursive_mutex_exit(&cyw43_mutex);
    } else {
        CYW43_STAT_INC(PENDSV_DISABLED_COUNT);
        low_priority_irq_missed = true;
    }
    sem_release(&cyw43_irq_sem);
}

static bool low_prio_irq_init(uint8_t priority) {
    assert(get_core_num() == cyw43_core_num);
    int irq = user_irq_claim_unused(false);
    if (irq < 0) return false;
    low_priority_irq_num = (uint8_t) irq;
    irq_set_exclusive_handler(low_priority_irq_num, low_priority_irq_handler);
    irq_set_enabled(low_priority_irq_num, true);
    irq_set_priority(low_priority_irq_num, priority);
    return true;
}

static void low_prio_irq_deinit(void) {
    if (low_priority_irq_num > 0) {
        irq_set_enabled(low_priority_irq_num, false);
        irq_remove_handler(low_priority_irq_num, low_priority_irq_handler);
        user_irq_unclaim(low_priority_irq_num);
        low_priority_irq_num = 0;
    }
}

int cyw43_arch_init(void) {
    cyw43_core_num = get_core_num();
    recursive_mutex_init(&cyw43_mutex);
    cyw43_init(&cyw43_state);
    sem_init(&cyw43_irq_sem, 0, 1);

    // Start regular lwip callback to handle timeouts
    periodic_alarm = add_alarm_in_us(CYW43_SLEEP_CHECK_MS * 1000, periodic_alarm_handler, NULL, true);
    if (periodic_alarm < 0) {
        return PICO_ERROR_GENERIC;
    }

    gpio_add_raw_irq_handler_with_order_priority(IO_IRQ_BANK0, gpio_irq_handler, CYW43_GPIO_IRQ_HANDLER_PRIORITY);
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

#if CYW43_LWIP
    lwip_init();
#endif
    // start low priority handler (no background work is done before this)
    bool ok = low_prio_irq_init(PICO_LOWEST_IRQ_PRIORITY);
    if (!ok) {
        cyw43_arch_deinit();
        return PICO_ERROR_GENERIC;
    }
    return PICO_OK;
}

void cyw43_arch_deinit(void) {
    if (periodic_alarm >= 0) {
        cancel_alarm(periodic_alarm);
        periodic_alarm = -1;
    }
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, false);
    gpio_remove_raw_irq_handler(IO_IRQ_BANK0, gpio_irq_handler);
    low_prio_irq_deinit();
    cyw43_deinit(&cyw43_state);
}

void cyw43_post_poll_hook(void) {
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
}

// This is called in the gpio and low_prio_irq interrupts and on either core
static void low_prio_irq_schedule_dispatch(size_t slot, low_prio_irq_dispatch_t f) {
    assert(slot < count_of(low_priority_irq_dispatch_slots));
    low_priority_irq_dispatch_slots[slot] = f;
    if (cyw43_core_num == get_core_num()) {
        //on same core, can dispatch directly
        irq_set_pending(low_priority_irq_num);
    } else {
        // on wrong core, so force via GPIO IRQ which itself calls this method for the CYW43 slot.
        // since the CYW43 slot always uses the same function, this is fine with the addition of an
        // extra (but harmless) CYW43 slot call when another SLOT is invoked.
        // We could do better, but would have to track why the IRQ was called.
        io_irq_ctrl_hw_t *irq_ctrl_base = cyw43_core_num ?
                                          &iobank0_hw->proc1_irq_ctrl : &iobank0_hw->proc0_irq_ctrl;
        hw_set_bits(&irq_ctrl_base->intf[CYW43_PIN_WL_HOST_WAKE/8], GPIO_IRQ_LEVEL_HIGH << (4 * (CYW43_PIN_WL_HOST_WAKE & 7)));
    }
}

void cyw43_schedule_internal_poll_dispatch(void (*func)(void)) {
    low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_CYW43, func);
}

// Prevent background processing in pensv and access by the other core
// These methods are called in pensv context and on either core
// They can be called recursively
void cyw43_thread_enter(void) {
    // Lock the other core and stop low_prio_irq running
    recursive_mutex_enter_blocking(&cyw43_mutex);
}

#ifndef NDEBUG
void cyw43_thread_lock_check(void) {
    // Lock the other core and stop low_prio_irq running
    if (recursive_mutex_enter_count(&cyw43_mutex) < 1 || recursive_mutex_owner(&cyw43_mutex) != lock_get_caller_owner_id()) {
        panic("cyw43_thread_lock_check failed");
    }
}
#endif

// Re-enable background processing
void cyw43_thread_exit(void) {
    // Run low_prio_irq if needed
    if (1 == recursive_mutex_enter_count(&cyw43_mutex)) {
        // note the outer release of the mutex is not via cyw43_exit in the low_priority_irq case (it is a direct mutex exit)
        assert(!in_low_priority_irq);
//        if (low_priority_irq_missed) {
//            low_priority_irq_missed = false;
            if (low_priority_irq_dispatch_slots[CYW43_DISPATCH_SLOT_CYW43]) {
                low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_CYW43, cyw43_poll);
            }
//        }
    }
    recursive_mutex_exit(&cyw43_mutex);
}


static void cyw43_delay_until(absolute_time_t until) {
    // sleep can be called in IRQs, so there's not much we can do there
    if (__get_current_exception()) {
        busy_wait_until(until);
    } else {
        sleep_until(until);
    }
}

void cyw43_delay_ms(uint32_t ms) {
    cyw43_delay_until(make_timeout_time_ms(ms));
}

void cyw43_delay_us(uint32_t us) {
    cyw43_delay_until(make_timeout_time_us(us));
}

void cyw43_arch_poll() {
    // should not be necessary
//    if (cyw43_poll) {
//        low_prio_irq_schedule_dispatch(CYW43_DISPATCH_SLOT_CYW43, cyw43_poll);
//    }
}

#if !CYW43_LWIP
static void no_lwip_fail() {
    panic("You cannot use IP with pico_cyw43_arch_none");
}
void cyw43_cb_tcpip_init(cyw43_t *self, int itf) {
}
void cyw43_cb_tcpip_deinit(cyw43_t *self, int itf) {
}
void cyw43_cb_tcpip_set_link_up(cyw43_t *self, int itf) {
    no_lwip_fail();
}
void cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf) {
    no_lwip_fail();
}
void cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len, const uint8_t *buf) {
    no_lwip_fail();
}
#endif

#endif