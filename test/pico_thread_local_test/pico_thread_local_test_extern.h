/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_THREAD_LOCAL_TEST_EXTERN_H
#define _PICO_THREAD_LOCAL_TEST_EXTERN_H

// We define a thread local defined in a second translation unit, as the compiler treats thread locals
// defined in the same translation unit differently (without -ftls-model=local-exec, which
// we should now have, but still no harm in adding this test)

#ifdef __cplusplus
extern "C" {
#endif

#define EXTERN_COUNTER_INIT_VALUE 11

extern __thread int extern_counter;

// bumps extern_counter from the TU it is defined in, so both directions are covered
int extern_bump(int delta);

#ifdef __cplusplus
}
#endif

#endif
