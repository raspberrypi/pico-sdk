/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Probe: can FreeRTOS event-bit aliasing lose a lock_core wakeup?
 *
 * The RP2040/RP2350 FreeRTOS ports implement the pico-sdk lock_core wait/notify primitives with
 * a single event group, mapping spin lock number -> event bit in prvGetEventGroupBit():
 *
 *     32-bit ticks: bit = spin_lock_num % 24
 *     16-bit ticks: bit = spin_lock_num & 0x7
 *
 * so two different lock_core instances can share a bit. The wait is
 *
 *     spin_unlock( pxLock->spin_lock, ulSave );            <-- window opens here
 *     xEventGroupWaitBits( ..., xClearOnExit=pdTRUE, ... );
 *
 * and xEventGroupSetBits() unblocks every task waiting on the bit and then clears it. So a
 * notify for lock V landing while task VICTIM is inside that window - not yet on the event
 * list - can unblock THIEF, waiting on an aliased lock T, and clear the bit. THIEF rechecks its
 * own condition, re-waits, and never touches lock V, so never re-sets the bit; VICTIM then
 * blocks with its permit already granted and nobody left to notify it.
 *
 * The port's comment in vPortLockInternalSpinUnlockWithWait() argues this cannot happen, since
 * "any intervening blocked lock ... will need to unlock it before we proceed, which will set
 * the event bit again". That holds for a thief on the same lock, but not obviously for one on
 * an aliased lock.
 *
 * Method: VICTIM repeatedly does sem_acquire_blocking() on a semaphore whose permit is granted
 * from a timer IRQ at a swept delay, so the release walks across the window. THIEF sits
 * permanently blocked on a second semaphore, and a monitor task rescues a stalled iteration so
 * the run continues and can be counted. A CONTROL pass with THIEF on a non-aliased lock runs
 * first and must show zero stalls, or the harness is at fault and the aliased number means
 * nothing.
 *
 * ON A NEGATIVE RESULT: the window runs only from spin_unlock() to the vTaskSuspendAll() inside
 * xEventGroupWaitBits(), a handful of instructions. Zero stalls means "not reproduced", NOT
 * "proven impossible".
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/time.h"

#include "FreeRTOS.h"
#include "task.h"

/* ---- event bit mapping, mirroring prvGetEventGroupBit() in the port ------------------ */
#if ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
#define EVENT_BIT_OF(n)     ( ( n ) & 0x7u )
#define NUM_EVENT_BITS      8u
#else
#define EVENT_BIT_OF(n)     ( ( n ) % 24u )
#define NUM_EVENT_BITS      24u
#endif

/* Spin locks are chosen by hand so we control aliasing, rather than relying on whatever
 * next_striped_spin_lock_num() happens to hand out. LOCK_VICTIM + 24 aliases under either
 * tick width (24 % 24 == 0, and 24 & 7 == 0). */
#define LOCK_VICTIM         2u
#define LOCK_THIEF_ALIAS    ( LOCK_VICTIM + 24u )
#define LOCK_THIEF_CONTROL  3u

static_assert(EVENT_BIT_OF(LOCK_THIEF_ALIAS) == EVENT_BIT_OF(LOCK_VICTIM),
              "alias pair must share an event bit");
static_assert(EVENT_BIT_OF(LOCK_THIEF_CONTROL) != EVENT_BIT_OF(LOCK_VICTIM),
              "control pair must not share an event bit");
static_assert(LOCK_THIEF_ALIAS < NUM_SPIN_LOCKS, "alias lock out of range");

/* ---- tuning ------------------------------------------------------------------------- */
#define ITERATIONS          4000u   /* per pass */
#define SWEEP_US            97u     /* alarm delay sweeps 1..SWEEP_US microseconds */
#define JITTER_CYCLES       64u     /* sub-microsecond phase jitter */
#define STALL_MS            150     /* an iteration slower than this counts as stalled */
#define MAX_STALLS          20u     /* stop a pass early once the point is made */
#define MONITOR_POLL_MS     5

#define MAIN_PRIORITY       ( tskIDLE_PRIORITY + 1UL )
#define VICTIM_PRIORITY     ( tskIDLE_PRIORITY + 2UL )
#define THIEF_PRIORITY      ( tskIDLE_PRIORITY + 2UL )
#define MONITOR_PRIORITY    ( tskIDLE_PRIORITY + 3UL )

static semaphore_t sem_victim;
static semaphore_t sem_thief;

static volatile bool     iteration_running;
static volatile uint64_t iteration_start_us;
static volatile uint32_t stalls;
static volatile uint32_t completed;
static volatile bool     pass_done;

/* Granted from the timer IRQ -- an entirely ordinary use of sem_release(). */
static int64_t release_victim(__unused alarm_id_t id, __unused void *user_data) {
    sem_release(&sem_victim);
    return 0;
}

/* Sits blocked forever on an unrelated semaphore. Each time the shared event bit is set it
 * is woken, finds no permit, and re-waits -- clearing the bit on the way through. */
static void thief_task(__unused void *params) {
    sem_acquire_blocking(&sem_thief);   /* never satisfied */
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

static void monitor_task(__unused void *params) {
    while (!pass_done) {
        vTaskDelay(pdMS_TO_TICKS(MONITOR_POLL_MS));
        if (iteration_running &&
            absolute_time_diff_us(from_us_since_boot(iteration_start_us),
                                  get_absolute_time()) > (int64_t)STALL_MS * 1000) {
            /* VICTIM is still blocked although its permit was granted long ago. */
            iteration_running = false;
            stalls++;
            sem_release(&sem_victim);   /* rescue: VICTIM is on the event list by now */
        }
    }
    vTaskDelete(NULL);
}

static void victim_task(__unused void *params) {
    for (uint32_t i = 0; i < ITERATIONS && stalls < MAX_STALLS; i++) {
        sem_reset(&sem_victim, 0);

        /* Sweep the release across the window, including sub-microsecond phase. */
        busy_wait_at_least_cycles(i % JITTER_CYCLES);
        uint32_t delay_us = 1u + (i % SWEEP_US);

        iteration_start_us = to_us_since_boot(get_absolute_time());
        iteration_running = true;
        if (add_alarm_in_us(delay_us, release_victim, NULL, true) <= 0) {
            printf("  FAILED to add alarm\n");
            break;
        }

        sem_acquire_blocking(&sem_victim);
        iteration_running = false;
        completed++;
    }
    pass_done = true;
    vTaskDelete(NULL);
}

/* Returns the number of stalled iterations. */
static uint32_t run_pass(const char *name, uint thief_lock_num) {
    bool aliased = EVENT_BIT_OF(thief_lock_num) == EVENT_BIT_OF(LOCK_VICTIM);
    printf("\n--- pass: %s ---\n", name);
    printf("  victim lock %2u -> event bit %2u\n",
           LOCK_VICTIM, (uint)EVENT_BIT_OF(LOCK_VICTIM));
    printf("  thief  lock %2u -> event bit %2u   (%s)\n",
           thief_lock_num, (uint)EVENT_BIT_OF(thief_lock_num),
           aliased ? "ALIASED" : "distinct");

    sem_init(&sem_victim, 0, 16);
    sem_init(&sem_thief, 0, 16);
    /* override the striped spin locks sem_init chose, with our chosen numbers */
    lock_init(&sem_victim.core, LOCK_VICTIM);
    lock_init(&sem_thief.core, thief_lock_num);

    stalls = 0;
    completed = 0;
    iteration_running = false;
    pass_done = false;

    TaskHandle_t thief;
    xTaskCreate(thief_task, "thief", configMINIMAL_STACK_SIZE, NULL, THIEF_PRIORITY, &thief);
    xTaskCreate(monitor_task, "monitor", configMINIMAL_STACK_SIZE, NULL, MONITOR_PRIORITY, NULL);
    /* let the thief actually reach its blocking wait before we start */
    vTaskDelay(pdMS_TO_TICKS(50));
    xTaskCreate(victim_task, "victim", configMINIMAL_STACK_SIZE, NULL, VICTIM_PRIORITY, NULL);

    while (!pass_done) {
        vTaskDelay(pdMS_TO_TICKS(250));
        printf("    %u/%u iterations, %u stalls\n", completed, ITERATIONS, stalls);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    vTaskDelete(thief);

    printf("  result: %u stalled of %u completed iterations\n", stalls, completed);
    return stalls;
}

static void main_task(__unused void *params) {
    printf("\n=== FreeRTOS lock_core event-bit aliasing probe ===\n");
    printf("%u spin locks folded onto %u event bits (%d-bit ticks), %u cores\n",
           (uint)NUM_SPIN_LOCKS, NUM_EVENT_BITS,
           (int)configTICK_TYPE_WIDTH_IN_BITS, (uint)configNUMBER_OF_CORES);

    uint32_t control_stalls = run_pass("CONTROL (non-aliased thief)", LOCK_THIEF_CONTROL);
    uint32_t alias_stalls = run_pass("ALIASED thief", LOCK_THIEF_ALIAS);

    printf("\n=== summary ===\n");
    if (control_stalls) {
        printf("INVALID: the control pass stalled %u times, so either the harness is at\n"
               "fault or lock_core waits lose wakeups even without aliasing. The aliased\n"
               "number means nothing until that is understood.\n", control_stalls);
    } else if (alias_stalls) {
        printf("CONFIRMED: %u stalls with an aliased thief, 0 in the control.\n"
               "Event-bit aliasing can lose a lock_core wakeup -- a port bug.\n"
               "FAILED\n", alias_stalls);
    } else {
        printf("NOT REPRODUCED: 0 stalls in %u aliased iterations.\n"
               "The window is only spin_unlock() -> vTaskSuspendAll() inside\n"
               "xEventGroupWaitBits(), i.e. a few instructions, so this is evidence that\n"
               "the race is very hard to hit -- NOT proof that it cannot happen.\n"
               "PASSED\n", ITERATIONS);
    }
    // don't spin here: an automated harness would have to wait out its timeout to collect a
    // result that is already known
    exit((control_stalls || alias_stalls) ? 1 : 0);
}

int main(void) {
    stdio_init_all();
    xTaskCreate(main_task, "main", configMINIMAL_STACK_SIZE * 2, NULL, MAIN_PRIORITY, NULL);
    vTaskStartScheduler();
    return 0;
}
