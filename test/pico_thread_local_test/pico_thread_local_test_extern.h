/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_THREAD_LOCAL_TEST_EXTERN_H
#define _PICO_THREAD_LOCAL_TEST_EXTERN_H

// A thread local defined in a second translation unit. The compiler can see that a thread local
// defined in the same file is local to the executable and reaches it straight off the thread
// pointer; one it only has a declaration for, it cannot, so it goes via a GOT entry instead
// (initial-exec rather than local-exec). That is a different code path, a different section, and
// it is the one an application gets whenever it declares a thread local in a header - so it is
// worth a test having actually run it.

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
