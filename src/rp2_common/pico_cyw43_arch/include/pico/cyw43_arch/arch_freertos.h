/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_CYW43_ARCH_ARCH_FREERTOS_H
#define _PICO_CYW43_ARCH_ARCH_FREERTOS_H

#include "pico/cyw43_arch/arch_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// PICO_CONFIG: CYW43_TASK_STACK_SIZE, Stack size for the cyw43_task thread in 4byte words, type=int, default=1024, group=pico_cyw43_arch
#ifndef CYW43_TASK_STACK_SIZE
#define CYW43_TASK_STACK_SIZE 1024
#endif

// PICO_CONFIG: CYW43_TASK_PRIORITY, Priority for the cyw43_task thread, type=int default=4, group=pico_cyw43_arch
#ifndef CYW43_TASK_PRIORITY
#define CYW43_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#endif

#ifdef __cplusplus
}
#endif

#endif