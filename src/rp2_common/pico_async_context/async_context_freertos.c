/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "pico/async_context_freertos.h"
#include "pico/async_context_base.h"
#include "pico/sync.h"
#include "hardware/irq.h"

#include "semphr.h"

#if configNUMBER_OF_CORES > 1 && !defined(configUSE_CORE_AFFINITY)
#error async_context_freertos requires configUSE_CORE_AFFINITY under SMP
#endif

static const async_context_type_t template;

static void async_context_freertos_acquire_lock_blocking(async_context_t *self_base);
static void async_context_freertos_release_lock(async_context_t *self_base);
static void async_context_freertos_lock_check(async_context_t *self_base);

static TickType_t sensible_ticks_until(absolute_time_t until) {
    TickType_t ticks;
    int64_t delay_us = absolute_time_diff_us(get_absolute_time(), until);
    if (delay_us <= 0) {
        ticks = 0;
    } else {
        static const uint32_t max_delay = 60000000;
        uint32_t delay_us_32 = delay_us > max_delay ? max_delay : (uint32_t) delay_us;
        ticks = pdMS_TO_TICKS((delay_us_32+999)/1000);
        // we want to round up, as both rounding down to zero is wrong (may produce no delays
        // where delays are needed), but also we don't want to wake up, and then realize there
        // is no work to do yet!
        ticks++;
    }
    return ticks;
}

static void process_under_lock(async_context_freertos_t *self) {
#ifndef NDEBUG
    async_context_freertos_lock_check(&self->core);
#endif
    bool repeat;
    do {
        repeat = false;
        absolute_time_t next_time = async_context_base_execute_once(&self->core);
        TickType_t ticks;
        if (is_at_the_end_of_time(next_time)) {
            ticks = portMAX_DELAY;
        } else {
            ticks = sensible_ticks_until(next_time);
        }
        if (ticks) {
            // last parameter (timeout) is also 'ticks', since there is no point waiting to change the period
            // for longer than the period itself!
            repeat = pdFALSE == xTimerChangePeriod(self->timer_handle, ticks, ticks);
        } else {
            repeat = true;
        }
    } while (repeat);
}

static void async_context_task(__unused void *vself) {
    async_context_freertos_t *self = (async_context_freertos_t *)vself;
    do {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        if (self->task_should_exit) break;
        async_context_freertos_acquire_lock_blocking(&self->core);
        process_under_lock(self);
        async_context_freertos_release_lock(&self->core);
        __sev(); // it is possible regular code is waiting on a WFE on the other core
    } while (!self->task_should_exit);
    xSemaphoreGive(self->task_complete_sem);
    // Previously we called vTaskDelete(NULL) here, however this (self-delete) is always asynchronous and deferred
    // to the idle task, which is a problem with static allocation, as async_context_freertos_deinit zeroes
    // the context which actually contains the TCB, thus stomping on FreeRTOS's own internal (active) data structures,
    // before the idle task gets a chance to clean it up. async_context_freertos_deinit must therefore be responsible
    // for destroying this task in that case (and indeed it is simpler to do so there in all cases to make sure
    // the timer is quiesced too)

    // We must however do something other than return from a task, so we suspend ourself. Note however, that
    // there is no guarantee this code actually executes before async_context_freertos_deinit proceeds since
    // we have posted it a semaphore above, so the de-init code must not rely on this having happened.
    vTaskSuspend(NULL);
}

static void async_context_freertos_wake_up(async_context_t *self_base) {
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    TaskHandle_t task_handle = self->task_handle;
    if (task_handle) {
        if (portCHECK_IF_IN_ISR()) {
            vTaskNotifyGiveFromISR(task_handle, NULL);
            xSemaphoreGiveFromISR(self->work_needed_sem, NULL);
        } else {
            // we don't want to wake ourselves up (we will only ever be called
            // from the async_context_task if we own the lock, in which case processing
            // will already happen when the lock is finally unlocked
            if (xTaskGetCurrentTaskHandle() != task_handle) {
                xTaskNotifyGive(task_handle);
                xSemaphoreGive(self->work_needed_sem);
            } else {
#ifndef NDEBUG
                async_context_freertos_lock_check(self_base);
#endif
            }
        }
    }
}

static void timer_handler(__unused TimerHandle_t handle)
{
    async_context_freertos_t *self = (async_context_freertos_t *)pvTimerGetTimerID(handle);
    async_context_freertos_wake_up(&self->core);
}

bool async_context_freertos_init(async_context_freertos_t *self, async_context_freertos_config_t *config) {
    memset(self, 0, sizeof(*self));
    self->core.type = &template;
    self->core.flags = ASYNC_CONTEXT_FLAG_CALLBACK_FROM_NON_IRQ;
#if configNUMBER_OF_CORES > 1
    // sample the core once: core_num must match the core the task is pinned to
    UBaseType_t core_id = config->task_core_id;
    if (core_id == (UBaseType_t)-1) {
        core_id = portGET_CORE_ID();
    }
    assert(core_id < configNUMBER_OF_CORES);
    self->core.core_num = (uint8_t)core_id;
    const UBaseType_t core_affinity_mask = 1u << core_id;
#else
    self->core.core_num = get_core_num();
#endif
#if configSUPPORT_STATIC_ALLOCATION
    assert(config->task_stack);
    self->lock_mutex = xSemaphoreCreateRecursiveMutexStatic(&self->lock_mutex_buf);
    self->work_needed_sem = xSemaphoreCreateBinaryStatic(&self->work_needed_sem_buf);
    self->task_complete_sem = xSemaphoreCreateBinaryStatic(&self->task_complete_sem_buf);
    self->timer_handle = xTimerCreateStatic( "async_context_timer",       // Just a text name, not used by the kernel.
                                             portMAX_DELAY,
                                             pdFALSE,        // The timers will auto-reload themselves when they expire.
                                             self,
                                             timer_handler,
                                             &self->timer_buf);
#if configNUMBER_OF_CORES > 1
    // create the task pre-pinned so it can't run on the wrong core first
    self->task_handle = xTaskCreateStaticAffinitySet( async_context_task,
                                                      "async_context_task",
                                                      config->task_stack_size,
                                                      self,
                                                      config->task_priority,
                                                      config->task_stack,
                                                      &self->task_buf,
                                                      core_affinity_mask);
#else
    self->task_handle = xTaskCreateStatic( async_context_task,
                                           "async_context_task",
                                           config->task_stack_size,
                                           self,
                                           config->task_priority,
                                           config->task_stack,
                                           &self->task_buf);
#endif
#else
    self->lock_mutex = xSemaphoreCreateRecursiveMutex();
    self->work_needed_sem = xSemaphoreCreateBinary();
    self->task_complete_sem = xSemaphoreCreateBinary();
    self->timer_handle = xTimerCreate( "async_context_timer",       // Just a text name, not used by the kernel.
                                    portMAX_DELAY,
                                    pdFALSE,        // The timers will auto-reload themselves when they expire.
                                    self,
                                    timer_handler);
#endif

    if (!self->lock_mutex ||
        !self->work_needed_sem ||
        !self->timer_handle ||
#if configSUPPORT_STATIC_ALLOCATION
        !self->task_handle
#elif configNUMBER_OF_CORES > 1
        pdPASS != xTaskCreateAffinitySet(async_context_task, "async_context_task", config->task_stack_size, self,
                config->task_priority, core_affinity_mask, &self->task_handle)
#else
        pdPASS != xTaskCreate(async_context_task, "async_context_task", config->task_stack_size, self,
                config->task_priority, &self->task_handle)
#endif
    ) {
        async_context_deinit(&self->core);
        return false;
    }
    return true;
}

static uint32_t end_task_func(void *param) {
    async_context_freertos_t *self = (async_context_freertos_t *)param;
    // we will immediately exit
    self->task_should_exit = true;
    return 0;
}

static void timer_delete_sync_helper(__unused void *param1, __unused uint32_t param2) {
    TaskHandle_t xTaskToNotify = (TaskHandle_t)param1;
    xTaskNotifyGive(xTaskToNotify);
}

void async_context_freertos_deinit(async_context_t *self_base) {
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    TaskHandle_t task_handle = self->task_handle;
    // Ask the task to exit its loop, and wait for it to do so.
    if (task_handle) {
        async_context_execute_sync(self_base, end_task_func, self_base);
        if (self->task_complete_sem) {
            xSemaphoreTake(self->task_complete_sem, portMAX_DELAY);
        }
    }
    // Now the task has exited its loop, it will neither call worker functions nor re-arm
    // the timer. At this point though both:
    // a. The task may still be running on core (having not yet made it to the vTaskSuspend(NULL).
    // b. Be targeted for notifications by the timer which is still active. This is fine though
    //    as the task either is, or is about to be in vTaskSuspend(NULL) which isn't woken by notifications.

    // First things first, let's now synchronously stop the timer...
    if (self->timer_handle) {
        xTimerDelete(self->timer_handle, 0);

        // Slight hoops to jump thru to make sure the timer has actually been deleted BEFORE we proceed

        // 1. Queue function which will notify us back to the timer task queue
        xTimerPendFunctionCall(timer_delete_sync_helper, (void *)xTaskGetCurrentTaskHandle(), 0, portMAX_DELAY);
        // 2. Wait for that function to execute (which will be after the timer deletion completes)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        self->timer_handle = NULL;
    }

    // With the timer now stopped, there are no other current/future execution units referencing self->task_handle
    // and we can proceed to delete the task...
    if (task_handle) {
        // ... however vTaskDelete is only synchronous if the task is not currently executing (on core) during the call

        // 1. We don't care if not using static allocation, since nothing related to the task is stored in our
        //    soon to be zeroed context.
        // 2. We don't care if not using SMP since if the only core is currently executing vTaskDelete from this task,
        //   then clearly the other task is not currently executing
#if configSUPPORT_STATIC_ALLOCATION && ( configNUMBER_OF_CORES > 1 )
        // Make sure the task cannot be re-scheduled again. This is asynchronous across cores...
        vTaskSuspend(task_handle);
        // ... so actually wait until it actually leaves the core if it was on it
        while (xTaskGetCurrentTaskHandleForCore((BaseType_t)self->core.core_num) == task_handle) {
            taskYIELD();
        }
#endif
        // Now we can call vTaskDelete because in the static allocation case we know the task is
        // no longer executing, so this call is guaranteed to complete synchronously, and in the non
        // static allocation case we don't care if the call is asynchronous anyway.
        vTaskDelete(task_handle);
    }
    if (self->lock_mutex) {
        vSemaphoreDelete(self->lock_mutex);
    }
    if (self->work_needed_sem) {
        vSemaphoreDelete(self->work_needed_sem);
    }
    if (self->task_complete_sem) {
        vSemaphoreDelete(self->task_complete_sem);
    }
    // Finally clear the context now we know nothing is referencing it.
    memset(self, 0, sizeof(*self));
}

void async_context_freertos_acquire_lock_blocking(async_context_t *self_base) {
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    // Lock the other core and stop low_prio_irq running
    assert(!portCHECK_IF_IN_ISR());
    xSemaphoreTakeRecursive(self->lock_mutex, portMAX_DELAY);
    self->nesting++;
}

void async_context_freertos_lock_check(__unused async_context_t *self_base) {
#ifndef NDEBUG
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    // Lock the other core and stop low_prio_irq running
    assert(xSemaphoreGetMutexHolder(self->lock_mutex) == xTaskGetCurrentTaskHandle());
#endif
}

typedef struct sync_func_call{
    async_when_pending_worker_t worker;
    SemaphoreHandle_t sem;
#if configSUPPORT_STATIC_ALLOCATION
    StaticSemaphore_t sem_buf;
#endif
    uint32_t (*func)(void *param);
    void *param;
    uint32_t rc;
} sync_func_call_t;

static void handle_sync_func_call(async_context_t *context, async_when_pending_worker_t *worker) {
    sync_func_call_t *call = (sync_func_call_t *)worker;
    call->rc = call->func(call->param);
    xSemaphoreGive(call->sem);
}

static uint32_t async_context_freertos_execute_sync(async_context_t *self_base, uint32_t (*func)(void *param), void *param) {
    async_context_freertos_t *self = (async_context_freertos_t*)self_base;
    hard_assert(xSemaphoreGetMutexHolder(self->lock_mutex) != xTaskGetCurrentTaskHandle());
    sync_func_call_t call = {0};
    call.worker.do_work = handle_sync_func_call;
    call.func = func;
    call.param = param;
#if configSUPPORT_STATIC_ALLOCATION
    call.sem = xSemaphoreCreateBinaryStatic(&call.sem_buf);
#else
    call.sem = xSemaphoreCreateBinary();
#endif
    async_context_add_when_pending_worker(self_base, &call.worker);
    async_context_set_work_pending(self_base, &call.worker);
    xSemaphoreTake(call.sem, portMAX_DELAY);
    async_context_remove_when_pending_worker(self_base, &call.worker);
    vSemaphoreDelete(call.sem);
    return call.rc;
}

void async_context_freertos_release_lock(async_context_t *self_base) {
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    bool do_wakeup = false;
    if (self->nesting == 1) {
        // note that we always do a processing on outermost lock exit, to facilitate cases
        // like lwIP where we have no notification when lwIP timers are added.
        //
        // this operation must be done from the right task
        if (self->task_handle != xTaskGetCurrentTaskHandle()) {
            // note we defer the wakeup until after we release the lock, otherwise it can be wasteful
            // (waking up the task, but then having it block immediately on us)
            do_wakeup = true;
        } else {
            process_under_lock(self);
        }
    }
    --self->nesting;
    xSemaphoreGiveRecursive(self->lock_mutex);
    if (do_wakeup) {
        async_context_freertos_wake_up(self_base);
    }
}

static bool async_context_freertos_add_at_time_worker(async_context_t *self_base, async_at_time_worker_t *worker) {
    async_context_freertos_acquire_lock_blocking(self_base);
    bool rc = async_context_base_add_at_time_worker(self_base, worker);
    async_context_freertos_release_lock(self_base);
    return rc;
}

static bool async_context_freertos_remove_at_time_worker(async_context_t *self_base, async_at_time_worker_t *worker) {
    async_context_freertos_acquire_lock_blocking(self_base);
    bool rc = async_context_base_remove_at_time_worker(self_base, worker);
    async_context_freertos_release_lock(self_base);
    return rc;
}

static bool async_context_freertos_add_when_pending_worker(async_context_t *self_base, async_when_pending_worker_t *worker) {
    async_context_freertos_acquire_lock_blocking(self_base);
    bool rc = async_context_base_add_when_pending_worker(self_base, worker);
    async_context_freertos_release_lock(self_base);
    return rc;
}

static bool async_context_freertos_remove_when_pending_worker(async_context_t *self_base, async_when_pending_worker_t *worker) {
    async_context_freertos_acquire_lock_blocking(self_base);
    bool rc = async_context_base_remove_when_pending_worker(self_base, worker);
    async_context_freertos_release_lock(self_base);
    return rc;
}

static void async_context_freertos_set_work_pending(async_context_t *self_base, async_when_pending_worker_t *worker) {
    worker->work_pending = true;
    async_context_freertos_wake_up(self_base);
}

static void async_context_freertos_wait_until(__unused async_context_t *self_base, absolute_time_t until) {
    assert(!portCHECK_IF_IN_ISR());
    TickType_t ticks = sensible_ticks_until(until);
    vTaskDelay(ticks);
}

static void async_context_freertos_wait_for_work_until(async_context_t *self_base, absolute_time_t until) {
    async_context_freertos_t *self = (async_context_freertos_t *)self_base;
    assert(!portCHECK_IF_IN_ISR());
    while (!time_reached(until)) {
        TickType_t ticks = sensible_ticks_until(until);
        if (!ticks || xSemaphoreTake(self->work_needed_sem, ticks)) return;
    }
}

static const async_context_type_t template = {
        .type = ASYNC_CONTEXT_FREERTOS,
        .acquire_lock_blocking = async_context_freertos_acquire_lock_blocking,
        .release_lock = async_context_freertos_release_lock,
        .lock_check = async_context_freertos_lock_check,
        .execute_sync = async_context_freertos_execute_sync,
        .add_at_time_worker = async_context_freertos_add_at_time_worker,
        .remove_at_time_worker = async_context_freertos_remove_at_time_worker,
        .add_when_pending_worker = async_context_freertos_add_when_pending_worker,
        .remove_when_pending_worker = async_context_freertos_remove_when_pending_worker,
        .set_work_pending = async_context_freertos_set_work_pending,
        .poll = 0,
        .wait_until = async_context_freertos_wait_until,
        .wait_for_work_until = async_context_freertos_wait_for_work_until,
        .deinit = async_context_freertos_deinit,
};
