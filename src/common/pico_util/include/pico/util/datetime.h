/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_UTIL_DATETIME_H
#define _PICO_UTIL_DATETIME_H

#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file datetime.h
 * \defgroup util_datetime datetime
 * \brief Date/Time formatting
 * \ingroup pico_util
 */

#include <time.h>
#include <sys/time.h>

#if PICO_INCLUDE_RTC_DATETIME

/*! \brief  Convert a datetime_t structure to a string
 *  \ingroup util_datetime
 *
 * \param buf character buffer to accept generated string
 * \param buf_size The size of the passed in buffer
 * \param t The datetime to be converted.
 */
void datetime_to_str(char *buf, uint buf_size, const datetime_t *t);

bool time_to_datetime(time_t time, datetime_t *dt);
bool datetime_to_time(const datetime_t *dt, time_t *time);

void datetime_to_tm(const datetime_t *dt, struct tm *tm);
void tm_to_datetime(const struct tm *tm, datetime_t *dt);

#endif

uint64_t timespec_to_ms(const struct timespec *ts);
uint64_t timespec_to_us(const struct timespec *ts);
void ms_to_timespec(uint64_t ms, struct timespec *ts);
void us_to_timespec(uint64_t ms, struct timespec *ts);

/*! \brief  localtime_r implementation for use by the pico_util datetime functions
 *  \ingroup util_datetime
 *
 * This method calls localtime_r from the C library by default,
 * but is declared as a weak implementation to allow user code to override it
 */
struct tm *pico_localtime_r(const time_t *time, struct tm *tm);

/*! \brief  mktime implementation for use by the pico_util datetime functions
*  \ingroup util_datetime
*
* This method calls mktime from the C library by default,
* but is declared as a weak implementation to allow user code to override it
*/
time_t pico_mktime(struct tm *tm);

#ifdef __cplusplus
}
#endif
#endif
