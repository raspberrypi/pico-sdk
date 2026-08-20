/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Injected into every translation unit via PICO_PLATFORM_TEST_HEADER so that the wait loops
// inside pico_sync and pico_time can be recorded

#ifndef _INTEROP_TEST_HOOK_H
#define _INTEROP_TEST_HOOK_H

#ifndef __ASSEMBLER__
extern void interop_wait_wakeup(void);
#endif

#define blocked_waiter_wakeup interop_wait_wakeup

#endif
