/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <picotls.h>
#include "pico_thread_local_test_extern.h"

__thread int extern_counter = EXTERN_COUNTER_INIT_VALUE;

int extern_bump(int delta) {
    extern_counter += delta;
    return extern_counter;
}
