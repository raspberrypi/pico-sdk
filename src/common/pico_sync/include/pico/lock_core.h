/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_LOCK_CORE_H
#define _PICO_LOCK_CORE_H

#include "pico.h"
#include "pico/time.h"
#include "hardware/sync.h"

/** \file lock_core.h
 *  \defgroup lock_core lock_core
 *  \ingroup pico_sync
 * \brief base synchronization/lock primitive support.
 *
 * Most of the pico_sync locking primitives contain a lock_core_t structure member. This currently just holds a spin
 * lock which is used only to protect the contents of the rest of the structure as part of implementing the synchronization
 * primitive. As such, the spin_lock member of lock core is never still held on return from any function for the primitive.
 *
 * \ref critical_section is an exceptional case in that it does not have a lock_core_t and simply wraps a spin lock, providing
 * methods to lock and unlock said spin lock.
 *
 * lock_core based structures work by locking the spin lock, checking state, and then deciding whether they additionally need to block
 * or notify when the spin lock is released. In the blocking case, they will wake up again in the future, and try the process again.
 *
 * By default the SDK just uses the processors' events via SEV and WEV for notification and blocking as these are sufficient for
 * cross core, and notification from interrupt handlers. However macros are defined in this file that abstract the wait
 * and notify mechanisms to allow the SDK locking functions to effectively be used within an RTOS or other environment.
 *
 * When implementing an RTOS, it is desirable for the SDK synchronization primitives that wait, to block the calling task (and immediately yield),
 * and those that notify, to wake a blocked task which isn't on processor. At least the wait macro implementation needs to be atomic with the protecting
 * spin_lock unlock from the callers point of view; i.e. the task should unlock the spin lock when it starts its wait. Such implementation is
 * up to the RTOS integration, however the macros are defined such that such operations are always combined into a single call
 * (so they can be performed atomically) even though the default implementation does not need this, as a WFE which starts
 * following the corresponding SEV is not missed.
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_LOCK_CORE, Enable/disable assertions in the lock core, type=bool, default=0, group=pico_sync
#ifndef PARAM_ASSERTIONS_ENABLED_LOCK_CORE
#define PARAM_ASSERTIONS_ENABLED_LOCK_CORE 0
#endif

// PICO_CONFIG: PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND, Enable workaround to preserve low power waits in synchronization primitives where an exclusive access sets the calling core's own event, type=bool, default=1 when using software spin locks on such a platform, advanced=true, group=pico_sync
#ifndef PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND
#ifdef PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND
#define PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND PICO_SYNC_RP2350_SPIN_LOCK_WORKAROUND
#else
#define PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND (PICO_USE_SW_SPIN_LOCKS && PICO_EXCLUSIVE_ACCESS_SETS_OWN_EVENT)
#endif
#endif

/** \file lock_core.h
 *  \ingroup lock_core
 *
 * Base implementation for locking primitives protected by a spin lock. The spin lock is only used to protect
 * access to the remaining lock state (in primitives using lock_core); it is never left locked outside
 * of the function implementations
 */

/*! \brief Core state shared by all lock primitives
 *  \ingroup lock_core
 *
 * Contains the spin lock used to protect the internal state of a locking primitive.
 * The spin lock is released before any function returns; it is never held on exit.
 */
struct lock_core {
    spin_lock_t *spin_lock; ///< Spin lock protecting this lock's state

    // note any lock members in containing structures need not be volatile;
    // they are protected by memory/compiler barriers when gaining and release spin locks
};

typedef struct lock_core lock_core_t;

/*! \brief  Initialise a lock structure
 *  \ingroup lock_core
 *
 * Inititalize a lock structure, providing the spin lock number to use for protecting internal state.
 *
 * \param core Pointer to the lock_core to initialize
 * \param lock_num Spin lock number to use for the lock. As the spin lock is only used internally to the locking primitive
 *                 method implementations, this does not need to be globally unique, however could suffer contention
 */
void lock_init(lock_core_t *core, uint lock_num);

#ifndef lock_owner_id_t
/*! \brief  type to use to store the 'owner' of a lock.
 *  \ingroup lock_core
 *
 * By default this is int8_t as it only needs to store the core number or -1, however it may be
 * overridden if a larger type is required (e.g. for an RTOS task id)
 */
#define lock_owner_id_t int8_t
#endif

#ifndef LOCK_INVALID_OWNER_ID
/*! \brief  marker value to use for a lock_owner_id_t which does not refer to any valid owner
 *  \ingroup lock_core
 */
#define LOCK_INVALID_OWNER_ID ((lock_owner_id_t)-1)
#endif

#ifndef lock_get_caller_owner_id
/*! \brief  return the owner id for the caller
 *  \ingroup lock_core
 *
 * By default this returns the calling core number, but may be overridden (e.g. to return an RTOS task id)
 */
#define lock_get_caller_owner_id() ((lock_owner_id_t)get_core_num())
#ifndef lock_is_owner_id_valid
#define lock_is_owner_id_valid(id) ((id)>=0)
#endif
#endif

#ifndef lock_is_owner_id_valid
#define lock_is_owner_id_valid(id) ((id) != LOCK_INVALID_OWNER_ID)
#endif

#ifdef lock_internal_spin_unlock_with_wait
#define LOCK_INTERNAL_SPIN_UNLOCK_WITH_WAIT_OVERRIDDEN 1
#else
/*! \brief   Atomically unlock the lock's spin lock, and wait for a notification.
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for a concurrent lock_internal_spin_unlock_with_notify
 * to insert itself between the spin unlock and this wait in a way that the wait does not see the notification (i.e. causing
 * a missed notification). In other words this method should always wake up in response to a lock_internal_spin_unlock_with_notify
 * for the same lock, which completes after this call starts.
 *
 * In an ideal implementation, this method would return exactly after the corresponding lock_internal_spin_unlock_with_notify
 * has subsequently been called on the same lock instance, however this method is free to return at _any_ point before that;
 * this macro is _always_ used in a loop which locks the spin lock, checks the internal locking primitive state and then
 * waits again if the calling thread should not proceed.
 *
 * By default this macro simply unlocks the spin lock, and then performs a WFE, but may be overridden
 * (e.g. to actually block the RTOS task).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the `PRIMASK`
 *             state when the spin lock was acquire
 */
#if !PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND
#define lock_internal_spin_unlock_with_wait(lock, save) spin_unlock((lock)->spin_lock, save), __wfe()
#else
extern volatile uint8_t lock_internal_notify_count;
// Note the ordering here matters. The event register is a single bit, with multiple sources: our
// own spin_unlock, an SEV from a notifier, an SEV from some other unrelated code, or (on Arm) an
// exception entry + return on this core. We must drain the event left by the spin_unlock *before*
// deciding whether we still need to wait for a second event from a notifier.
//
// The first (draining) __wfe() is inside the lock's IRQ critical section. Return-from-interrupt
// generates an event on Armv8-M, but not on Hazard3 v1.0, so IRQs can swallow events on RISC-V.
// This first __wfe() always completes promptly, because spin_unlock_unsafe() generates an event.
//
// With the drain first, a notify occurring after _notify_count is sampled either: (a) bumps the
// count before we test it, so we skip the second __wfe(), or (b) happens after the test, in which
// case its SEV sets the event register and the second __wfe() returns immediately.
//
// (Also note: the increment cannot happen between the _notify_count and the spin_unlock_unsafe()
// because the increment must hold the lock *we are initially holding*.)
//
// See comment on `lock_internal_notify_count` declaration in lock_core.c for background details
// on interactions between events and exclusives on RP2350.
#define lock_internal_spin_unlock_with_wait(lock, save) ({         \
    uint8_t _notify_count = lock_internal_notify_count;            \
    spin_unlock_unsafe((lock)->spin_lock);                         \
    __wfe(); /* consume event from unlock, without interruption */ \
    restore_interrupts_from_disabled(save);                        \
    if (_notify_count == lock_internal_notify_count) __wfe();      \
    })
#endif
#endif

#ifdef lock_internal_spin_unlock_with_notify
#define LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_OVERRIDDEN 1
#else
/*! \brief   Atomically unlock the lock's spin lock, and send a notification
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for this notification to happen
 * during a lock_internal_spin_unlock_with_wait in a way that that wait does not see the
 * notification (i.e. causing a missed notification).
 *
 * Restating the above in terms of lock ordering: if lock_internal_spin_unlock_with_notify()
 * releases a lock acquired after lock_internal_spin_unlock_with_wait() released the same lock, the
 * waiter must be notified. Failure to notify can cause lockup. Excess notifications are harmless.
 *
 * The macro LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL records whether the implementation
 * upholds the above guarantee. It's set by default when the SDK's built-in implementation is used:
 * this is plain SEV/WFE on RP2040, and slightly more complex on RP2350 due to interactions between
 * events and exclusives. The macros injected by FreeRTOS ports currently do not uphold the
 * guarantee: they consume a shared event-group bit, so a waiter still between its spin unlock and
 * its block finds nothing left.
 *
 * By default this macro simply unlocks the spin lock, and then performs a SEV, but may be overridden
 * (e.g. to actually un-block RTOS task(s)).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the PRIMASK
 *             state when the spin lock was acquire)
 */
#if !PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND
#define lock_internal_spin_unlock_with_notify(lock, save) spin_unlock((lock)->spin_lock, save), __sev()
#else
// note that spin_lock_blocking() already posts an event to the current core: ldaexb/strex on Arm,
// amoor.w.aq on RISC-V, so creates + retires a reservation. (spin_unlock() doesn't as it's just a
// release-ordered store, but the point is a lock+unlock will always cause a core-local event.)
#define lock_internal_spin_unlock_with_notify(lock, save) ({ \
    lock_internal_notify_count++;                            \
    spin_unlock((lock)->spin_lock, save);                    \
    __sev();                                                 \
    })
#endif
#endif

#ifdef lock_internal_spin_unlock_with_best_effort_wait_or_timeout
#define LOCK_INTERNAL_SPIN_UNLOCK_WITH_BEST_EFFORT_WAIT_OR_TIMEOUT_OVERRIDDEN 1
#else
/*! \brief   Atomically unlock the lock's spin lock, and wait for a notification or a timeout
 *  \ingroup lock_core
 *
 * _Atomic_ here refers to the fact that it should not be possible for a concurrent lock_internal_spin_unlock_with_notify
 * to insert itself between the spin unlock and this wait in a way that the wait does not see the notification (i.e. causing
 * a missed notification). In other words this method should always wake up in response to a lock_internal_spin_unlock_with_notify
 * whose acquisition of the same lock is ordered after this method's lock release.
 *
 * In an ideal implementation, this method would return exactly after the corresponding lock_internal_spin_unlock_with_notify
 * has subsequently been called on the same lock instance or the timeout has been reached, however this method is free to return
 * at _any_ point before that; this macro is _always_ used in a loop which locks the spin lock, checks the internal locking
 * primitive state and then waits again if the calling thread should not proceed.
 *
 * By default this simply unlocks the spin lock, and then calls \ref best_effort_wfe_or_timeout
 * but may be overridden (e.g. to actually block the RTOS task with a timeout).
 *
 * \param lock the lock_core for the primitive which needs to block
 * \param save the uint32_t value that should be passed to spin_unlock when the spin lock is unlocked. (i.e. the PRIMASK
 *             state when the spin lock was acquire)
 * \param until the \ref absolute_time_t value
 * \return true if the timeout has been reached
 */
#if !PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND
#define lock_internal_spin_unlock_with_best_effort_wait_or_timeout(lock, save, until) ({ \
    spin_unlock((lock)->spin_lock, save);                                                \
    best_effort_wfe_or_timeout(until);                                                   \
})
#else
// see the comment on lock_internal_spin_unlock_with_wait above for why the event left by the
// spin_unlock must be drained before the notify count is tested
#define lock_internal_spin_unlock_with_best_effort_wait_or_timeout(lock, save, until) ({ \
    uint8_t _notify_count = lock_internal_notify_count;                                  \
    spin_unlock_unsafe((lock)->spin_lock);                                               \
    __wfe(); /* consume event from spin_unlock_unsafe() without interruption */          \
    restore_interrupts_from_disabled(save);                                              \
    _notify_count == lock_internal_notify_count ? best_effort_wfe_or_timeout(until)      \
                                                : time_reached(until);                   \
})
#endif
#endif

/*! \brief Whether a notify reaches all earlier waiters
 *  \ingroup lock_core
 *
 * The SDK's built-in notify/wait implementation guarantees the following: if
 * lock_internal_spin_unlock_with_wait() releases a lock, and that same lock is subsequently
 * acquired and then later released by lock_internal_spin_unlock_with_notify(), the waiter is
 * notified. **Only the lock acquisition order matters.** Even if there are multiple wait calls
 * before the notify call, all waiters are notified. Even if the waiter has released the lock but
 * not yet gone to sleep, once it sleeps, it must be woken.
 *
 * An RTOS override may not have that property. The FreeRTOS ports multiplex every spin lock onto
 * bits of one event group and consume the bit (xClearOnExit), so each notification is consumed by
 * exactly one waiter.
 *
 * If any notify/wait primitives are overridden, conservatively mark them as *not* upholding the
 * same guarantee. In this case we patch things up by emitting extra notifications so the first
 * waiter can wake the next waiter, and so on -- see lock_internal_spin_unlock_maybe_notify().
 *
 * You can redefine this to 1 if you are certain your implementations uphold the same contract as
 * the SDK versions.
 */
#ifndef LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL
#if !(LOCK_INTERNAL_SPIN_UNLOCK_WITH_WAIT_OVERRIDDEN | LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_OVERRIDDEN | LOCK_INTERNAL_SPIN_UNLOCK_WITH_BEST_EFFORT_WAIT_OR_TIMEOUT_OVERRIDDEN)
#define LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL 1
#else
#define LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL 0
#endif
#endif

/*! \brief Release a spin lock, notifying other waiters if the implementation does not guarantee
 *         that an earlier notification will have reached them.
 *  \ingroup lock_core
 *
 * The purpose of this primitive is: if multiple waiters are the target of a single notification,
 * but the implementation does not guarantee that the notification reaches all waiters
 * (so LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL = 0), *generate additional notifications* on
 * the first waiter's lock release, to propagate the original notification to the remaining
 * waiters.
 *
 * If LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL = 1 then no extra propagation is required, so
 * it's just a regular spin_unlock().
 *
 * The others_may_proceed parameter controls whether a notification is generated. For example, when
 * a semaphore has multiple outstanding permits, the first waiter consumes a permit and then passes
 * others_may_proceed = true, which notifies the next waiter. The chain of notifications continues
 * until others_may_proceed = false is passed: in this example, when the number of outstanding
 * semaphore permits reaches zero.
 *
 * \param lock the lock_core
 * \param save the uint32_t value returned by the corresponding spin_lock_blocking()
 * \param others_may_proceed true if another waiter could still proceed - i.e. this caller has
 *                            not excluded them
 */
#if LOCK_INTERNAL_SPIN_UNLOCK_WITH_NOTIFY_WAKES_ALL
#define lock_internal_spin_unlock_maybe_notify(lock, save, others_may_proceed) spin_unlock((lock)->spin_lock, save)
#else
#define lock_internal_spin_unlock_maybe_notify(lock, save, others_may_proceed) ({ \
    if (others_may_proceed) {                                                     \
        lock_internal_spin_unlock_with_notify(lock, save);                        \
    } else {                                                                      \
        spin_unlock((lock)->spin_lock, save);                                     \
    }                                                                             \
})
#endif

#ifndef sync_internal_yield_until_before
/*! \brief   yield to other processing until some time before the requested time
 *  \ingroup lock_core
 *
 * This method is provided for cases where the caller has no useful work to do
 * until the specified time.
 *
 * By default this method does nothing, however it can be overridden (for example by an
 * RTOS which is able to block the current task until the scheduler tick before
 * the given time)
 *
 * \param until the \ref absolute_time_t value
 */
#define sync_internal_yield_until_before(until) ((void)0)
#endif

#endif
