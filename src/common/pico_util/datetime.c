#include "pico/util/datetime.h"

#if !PICO_ON_DEVICE && __APPLE__
// if we're compiling with LLVM on Apple, __weak does something else, but we don't care about overriding these anyway on host builds
#define __datetime_weak
#else
#define __datetime_weak __weak
#endif

__datetime_weak struct tm * pico_localtime_r(const time_t *time, struct tm *tm) {
    return localtime_r(time, tm);
}

__datetime_weak time_t pico_mktime(struct tm *tm) {
    return mktime(tm);
}

#if PICO_INCLUDE_RTC_DATETIME
#include <stdio.h>

static const char *DATETIME_MONTHS[12] = {
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
};

static const char *DATETIME_DOWS[7] = {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
};

void datetime_to_str(char *buf, uint buf_size, const datetime_t *t) {
    snprintf(buf,
             buf_size,
             "%s %d %s %d:%02d:%02d %d",
             DATETIME_DOWS[t->dotw],
             t->day,
             DATETIME_MONTHS[t->month - 1],
             t->hour,
             t->min,
             t->sec,
             t->year);
};

void datetime_to_tm(const datetime_t *dt, struct tm *tm) {
    tm->tm_year = dt->year - 1900;
    tm->tm_mon = dt->month - 1;
    tm->tm_mday = dt->day;
    tm->tm_hour = dt->hour;
    tm->tm_min = dt->min;
    tm->tm_sec = dt->sec;
}

void tm_to_datetime(const struct tm *tm, datetime_t *dt) {
    dt->year = (int16_t) (tm->tm_year + 1900); // 0..4095
    dt->month = (int8_t) (tm->tm_mon + 1);     // 1..12, 1 is January
    dt->day = (int8_t) tm->tm_mday;            // 1..28,29,30,31 depending on month
    dt->dotw = (int8_t) tm->tm_wday;           // 0..6, 0 is Sunday
    dt->hour = (int8_t) tm->tm_hour;           // 0..23
    dt->min = (int8_t) tm->tm_min;             // 0..59
    dt->sec = (int8_t) tm->tm_sec;             // 0..59
}

bool time_to_datetime(time_t time, datetime_t *dt) {
    struct tm local;
    if (pico_localtime_r(&time, &local)) {
        tm_to_datetime(&local, dt);
        return true;
    }
    return false;
}

bool datetime_to_time(const datetime_t *dt, time_t *time) {
    struct tm local;
    datetime_to_tm(dt, &local);
    *time = pico_mktime(&local);
    return *time >= 0;
}

#endif

uint64_t timespec_to_ms(const struct timespec *ts) {
    int64_t rc = ts->tv_sec * 1000;
    rc += ts->tv_nsec / 1000000;
    return (uint64_t) rc;
}

void ms_to_timespec(uint64_t ms, struct timespec *ts) {
    ts->tv_sec = (time_t)((int64_t)ms / 1000);
    ts->tv_nsec = ((long)((int64_t)ms % 1000)) * 1000000;
}

uint64_t timespec_to_us(const struct timespec *ts) {
    int64_t rc = ts->tv_sec * 1000000;
    rc += ts->tv_nsec / 1000;
    return (uint64_t) rc;
}

void us_to_timespec(uint64_t ms, struct timespec *ts) {
    ts->tv_sec = (time_t)((int64_t)ms / 1000000);
    ts->tv_nsec = ((long)((int64_t)ms % 1000000)) * 1000;
}

