/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/cyw43_arch.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "cyw43_stats.h"

#if CYW43_LWIP
#include <lwip/tcpip.h>
#endif

#if PICO_CYW43_ARCH_FREERTOS

// FreeRTOS includes
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

#if NO_SYS
#error example_cyw43_arch_frertos_sys requires NO_SYS=0
#endif

#ifndef CYW43_TASK_PRIORITY
#define CYW43_TASK_PRIORITY ( tskIDLE_PRIORITY + 4)
#endif

#ifndef CYW43_SLEEP_CHECK_MS
#define CYW43_SLEEP_CHECK_MS 50 // How often to run lwip callback
#endif

#define CYW43_GPIO_IRQ_HANDLER_PRIORITY 0x40

static void signal_cyw43_task(void);

#if !LWIP_TCPIP_CORE_LOCKING_INPUT
static SemaphoreHandle_t cyw43_mutex;
#endif
static TimerHandle_t timer_handle;
static TaskHandle_t cyw43_task_handle;
static volatile bool cyw43_task_should_exit;
static SemaphoreHandle_t cyw43_worker_ran_sem;
static uint8_t cyw43_core_num;

// Called in low priority pendsv interrupt only to do lwip processing and check cyw43 sleep
static void periodic_worker(__unused TimerHandle_t handle)
{
#if CYW43_USE_STATS
    static uint32_t counter;
    if (counter++ % (30000 / LWIP_SYS_CHECK_MS) == 0) {
        cyw43_dump_stats();
    }
#endif

    CYW43_STAT_INC(LWIP_RUN_COUNT);
    if (cyw43_poll) {
        if (cyw43_sleep > 0) {
            if (--cyw43_sleep == 0) {
                signal_cyw43_task();
            }
        }
    }
}

void cyw43_await_background_or_timeout_us(uint32_t timeout_us) {
    // if we are called from within an IRQ, then don't wait (we are only ever called in a polling loop)
    assert(!portCHECK_IF_IN_ISR());
    xSemaphoreTake(cyw43_worker_ran_sem, pdMS_TO_TICKS(timeout_us / 1000));
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
        signal_cyw43_task();
        CYW43_STAT_INC(IRQ_COUNT);
    }
}

// Low priority interrupt handler to perform background processing
static void cyw43_task(__unused void *param) {
    do {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        if (cyw43_task_should_exit) break;
        cyw43_thread_enter();
        if (cyw43_poll) cyw43_poll();
        cyw43_thread_exit();
        xSemaphoreGive(cyw43_worker_ran_sem);
        __sev(); // it is possible regular code is waiting on a WFE on the other core
    } while (true);
}

static void tcpip_init_done(void *param) {
    xSemaphoreGive((SemaphoreHandle_t)param);
}

int cyw43_arch_init(void) {
    cyw43_core_num = get_core_num();
#if configUSE_CORE_AFFINITY && configNUM_CORES > 1
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    UBaseType_t affinity = vTaskCoreAffinityGet(task_handle);
    // we must bind the main task to one core during init
    vTaskCoreAffinitySet(task_handle, 1 << portGET_CORE_ID());
#endif
#if !LWIP_TCPIP_CORE_LOCKING_INPUT
    cyw43_mutex = xSemaphoreCreateRecursiveMutex();
#endif
    cyw43_init(&cyw43_state);
    cyw43_worker_ran_sem = xSemaphoreCreateBinary();

#if CYW43_LWIP
    SemaphoreHandle_t init_sem = xSemaphoreCreateBinary();
    tcpip_init(tcpip_init_done, init_sem);
    xSemaphoreTake(init_sem, portMAX_DELAY);
#endif

    timer_handle = xTimerCreate(    "cyw43_sleep_timer",       // Just a text name, not used by the kernel.
                                    pdMS_TO_TICKS(CYW43_SLEEP_CHECK_MS),
                                    pdTRUE,        // The timers will auto-reload themselves when they expire.
                                    NULL,
                                    periodic_worker);

    if (!timer_handle) {
        return PICO_ERROR_GENERIC;
    }

    gpio_add_raw_irq_handler_with_order_priority(IO_IRQ_BANK0, gpio_irq_handler, CYW43_GPIO_IRQ_HANDLER_PRIORITY);
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    cyw43_task_should_exit = false;
    xTaskCreate(cyw43_task, "CYW43 Worker", configMINIMAL_STACK_SIZE, NULL, CYW43_TASK_PRIORITY, &cyw43_task_handle);
#if configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // the cyw43 task mus tbe on the same core so it can restore IRQs
    vTaskCoreAffinitySet(cyw43_task_handle, 1 << portGET_CORE_ID());
#endif

#if configUSE_CORE_AFFINITY && configNUM_CORES > 1
    vTaskCoreAffinitySet(task_handle, affinity);
#endif

    return PICO_OK;
}

void cyw43_arch_deinit(void) {
    assert(cyw43_core_num == get_core_num());
    if (timer_handle) {
        xTimerDelete(timer_handle, 0);
        timer_handle = 0;
    }
    if (cyw43_task_handle) {
        cyw43_task_should_exit = true;
        signal_cyw43_task();
    }
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, false);
    gpio_remove_raw_irq_handler(IO_IRQ_BANK0, gpio_irq_handler);
}

void cyw43_post_poll_hook(void) {
    assert(cyw43_core_num == get_core_num());
    gpio_set_irq_enabled(CYW43_PIN_WL_HOST_WAKE, GPIO_IRQ_LEVEL_HIGH, true);
}

// This is called in the gpio and low_prio_irq interrupts and on either core
static void signal_cyw43_task(void) {
    if (cyw43_task_handle) {
        if (portCHECK_IF_IN_ISR()) {
            vTaskNotifyGiveFromISR(cyw43_task_handle, NULL);
        } else {
            xTaskNotifyGive(cyw43_task_handle);
        }
    }
}

void cyw43_schedule_internal_poll_dispatch(void (*func)(void)) {
    assert(func == cyw43_poll);
    signal_cyw43_task();
}

static int nesting;
// Prevent background processing in pensv and access by the other core
// These methods are called in pensv context and on either core
// They can be called recursively
void cyw43_thread_enter(void) {
    // Lock the other core and stop low_prio_irq running
    assert(!portCHECK_IF_IN_ISR());
#if LWIP_TCPIP_CORE_LOCKING_INPUT
    // we must share their mutex otherwise we can get deadlocks with two different recursive mutexes
    LOCK_TCPIP_CORE();
#else
    xSemaphoreTakeRecursive(cyw43_mutex, portMAX_DELAY);
#endif
    nesting++;
}

#ifndef NDEBUG
void cyw43_thread_lock_check(void) {
    // Lock the other core and stop low_prio_irq running
#if LWIP_TCPIP_CORE_LOCKING_INPUT
    assert(xSemaphoreGetMutexHolder(lock_tcpip_core.mut) == xTaskGetCurrentTaskHandle());
#else
    assert(xSemaphoreGetMutexHolder(cyw43_mutex) == xTaskGetCurrentTaskHandle());
#endif
}
#endif

// Re-enable background processing
void cyw43_thread_exit(void) {
    // Run low_prio_irq if needed
    --nesting;
#if LWIP_TCPIP_CORE_LOCKING_INPUT
    // we must share their mutex otherwise we can get deadlocks with two different recursive mutexes
    UNLOCK_TCPIP_CORE();
#else
    xSemaphoreGiveRecursive(cyw43_mutex);
#endif

    if (!nesting && cyw43_task_handle != xTaskGetCurrentTaskHandle())
        signal_cyw43_task();
}

void cyw43_delay_ms(uint32_t ms) {
    assert(!portCHECK_IF_IN_ISR());
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void cyw43_delay_us(uint32_t us) {
    if (us >= 1000) {
        cyw43_delay_ms(us / 1000);
    } else {
        vTaskDelay(1);
    }
}

void cyw43_arch_poll() {
}

#endif