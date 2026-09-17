/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/lock_core.h"

#if PICO_SYNC_EXCLUSIVE_ACCESS_EVENT_WORKAROUND
// Global rolling counter used to disambiguate ordering of SEV events. See definitions of:
// lock_internal_spin_unlock_with_wait(), lock_internal_spin_unlock_with_notify().
//
// Some background to understand why this is needed. Sources of events are:
//
// * All platforms: an SEV or __h3_unblock() from a different core/PE/hart.
//
// * Platforms with exclusives (RP2350): the current core's exclusive reservation at the Global
//   Monitor changes state from Exclusive to Open.
//
// The latter event is required by the Armv8-M architecture reference manual for all
// implementations. DDI0553 B9.3.1:
//
//     Whenever the global monitor state for a PE changes from Exclusive Access to Open Access, an
//     event is generated and held in the Event register for that PE. This register is used by the
//     Wait for Event mechanism.
//
// It is *implementation-defined* whether a PE's successful exclusive write to its own Global
// Monitor reservation causes a transition from Exclusive to Open. RP2350's implementation defines
// this to be the case. (see state transition diagram in B9.3.2 in the same spec.)
//
// The consequence is: **successful exclusive writes cause an event on the same core.** This
// includes `strex` instructions on Arm, and `sc.w` and `amo*.w` instructions on Hazard3's
// implementation of the RISC-V atomics extension. (Reservation loss due to another PE's
// non-exclusive write or successful exclusive write to the current PE's reservation also causes a
// monitor transition from Exclusive to Open, which again causes an event.)
//
// If spin locks are implemented using software atomics (which is the case on RP2350 due to
// RP2350-E2, "SIO SPINLOCK writes are mirrored at +0x80 offset") then acquiring and releasing a
// spin lock pings the owner with *at least* one event. This is problematic when the same event
// signal is used for cooperative sleep, hence this counter:
volatile uint8_t lock_internal_notify_count;
#endif

void lock_init(lock_core_t *core, uint lock_num) {
    valid_params_if(LOCK_CORE, lock_num < NUM_SPIN_LOCKS);
    core->spin_lock = spin_lock_instance(lock_num);
}

