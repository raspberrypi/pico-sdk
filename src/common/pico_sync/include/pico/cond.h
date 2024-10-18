/*
 * Copyright (c) 2022-2023 Paul Guyot <pguyot@kallisys.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PLATFORM_COND_H
#define _PLATFORM_COND_H

#include "pico/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file cond.h
 *  \defgroup cond cond
 *  \ingroup pico_sync
 * \brief Condition variable API for non IRQ mutual exclusion between cores
 *
 * Condition variables complement mutexes by providing a way to atomically
 * wait and release a held mutex. Then, the task on the other core can signal
 * the variable, which ends the wait. Often, the other core would also hold
 * the shared mutex, so the signaled task waits until the mutex is released.
 *
 * Condition variables can also be broadcast.
 *
 * In this implementation, it is not mandatory. The condition variables only
 * work with non-recursive mutexes.
 *
 * Limitations of mutexes also apply to condition variables. See \ref mutex.h
 */

typedef struct __packed_aligned
{
    lock_core_t core;
    lock_owner_id_t waiter;
    uint32_t broadcast_count;   // Overflow is unlikely
    bool signaled;
} cond_t;

/*! \brief Initialize a condition variable structure
 *  \ingroup cond
 *
 * \param cv Pointer to condition variable structure
 */
void cond_init(cond_t *cv);

/*! \brief Wait on a condition variable
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast. The mutex should
 * be owned and is released atomically. It is reacquired when this function
 * returns.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 */
void cond_wait(cond_t *cv, mutex_t *mtx);

/*! \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param until The time after which to return if the condition variable was
 * not signaled.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_until(cond_t *cv, mutex_t *mtx, absolute_time_t until);

/*! \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param timeout_ms The timeout in milliseconds.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_timeout_ms(cond_t *cv, mutex_t *mtx, uint32_t timeout_ms);

/*! \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param timeout_ms The timeout in microseconds.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_timeout_us(cond_t *cv, mutex_t *mtx, uint32_t timeout_us);

/*! \brief Signal on a condition variable and wake the waiter
 *  \ingroup cond
 *
 * \param cv Condition variable to signal
 */
void cond_signal(cond_t *cv);

/*! \brief Broadcast a condition variable and wake every waiters
 *  \ingroup cond
 *
 * \param cv Condition variable to signal
 */
void cond_broadcast(cond_t *cv);

#ifdef __cplusplus
}
#endif
#endif
