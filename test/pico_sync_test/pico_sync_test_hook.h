/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Injected into every translation unit of this test via PICO_PLATFORM_TEST_HEADER, so that the
// wait loops inside pico_sync and pico_time report each wakeup to the test.

#ifndef _PICO_SYNC_TEST_HOOK_H
#define _PICO_SYNC_TEST_HOOK_H

#ifndef __ASSEMBLER__
extern void pico_sync_test_wakeup(void);
#endif

#define blocked_waiter_wakeup pico_sync_test_wakeup

#endif
