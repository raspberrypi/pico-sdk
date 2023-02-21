/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if defined __ICCARM__

/* Used when you select "No-free heap" in Project > Options... > General options > Library options 2 */
#define PREFIX __no_free_
#include "pico_malloc.c"
#undef PREFIX
/* Used when you select "Basic heap" in Project > Options... > General options > Library options 2 */
#define PREFIX __basic_
#include "pico_malloc.c"
#undef PREFIX
/* Used when you select "Advanced heap" in Project > Options... > General options > Library options 2 */
#define PREFIX __iar_dl
#include "pico_malloc.c"
#undef PREFIX

#else

#error Unsupported toolchain

#endif
