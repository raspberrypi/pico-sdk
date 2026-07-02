/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_test_common.h"

// Set to 1 to continue after an error, 0 to exit
#if 0
#define EXIT_TEST
#else
#define EXIT_TEST return -1
#endif

bool repeater(repeating_timer_t *timer) {
    if (aon_timer_is_running()) {
        printf("  Repeating timer %d at %dms (aon: %dms)", *(uint32_t*)timer->user_data, to_ms_since_boot(get_absolute_time()), to_ms_since_boot(aon_timer_get_absolute_time()));
    } else {
        printf("  Repeating timer %d at %dms (aon: not running)", *(uint32_t*)timer->user_data, to_ms_since_boot(get_absolute_time()));
    }

#if PICO_NO_FLASH || PICO_COPY_TO_RAM
    printf("\n");
#else
    printf(" - Cache hit rate %.2f%%\n", ((float)xip_ctrl_hw->ctr_hit / (float)xip_ctrl_hw->ctr_acc) * 100.0f);
#endif

    status_led_set_state(!status_led_get_state());
    return true;
}

void chars_available_callback(__unused void *param) {
    char buf[16] = {0};
    while (stdio_get_until(buf, sizeof(buf), make_timeout_time_us(10)) > 0) {
        printf("Chars available callback: %s\n", buf);
    }
}

#if HAS_POWMAN_TIMER
static bool came_from_pstate = false;
static char powman_last_pwrup[100];
static char powman_last_pstate[100];

int __persistent_data(my_number);

// Increase this size to see the cache hit rate decrease, when using XIP_SRAM for persistent data
char __persistent_data(large_thing)[0x1000];

void pstate_resume_func(pstate_bitset_t *pstate) {
    came_from_pstate = true;
    memset(powman_last_pwrup, 0, sizeof(powman_last_pwrup));
    memset(powman_last_pstate, 0, sizeof(powman_last_pstate));
    switch (powman_hw->last_swcore_pwrup) {
        //               0 = chip reset, for the source of the last reset see
        case 1 << 0: strcpy(powman_last_pwrup, "Chip reset"); break;
        case 1 << 1: strcpy(powman_last_pwrup, "Pwrup0"); break;
        case 1 << 2: strcpy(powman_last_pwrup, "Pwrup1"); break;
        case 1 << 3: strcpy(powman_last_pwrup, "Pwrup2"); break;
        case 1 << 4: strcpy(powman_last_pwrup, "Pwrup3"); break;
        case 1 << 5: strcpy(powman_last_pwrup, "Coresight_pwrup"); break;
        case 1 << 6: strcpy(powman_last_pwrup, "Alarm_pwrup"); break;
        default: strcpy(powman_last_pwrup, "Unknown pwrup"); break;
    }

    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_XIP_CACHE)) strcat(powman_last_pstate, "XIP_CACHE, ");
    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK0)) strcat(powman_last_pstate, "SRAM_BANK0, ");
    if (pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK1)) strcat(powman_last_pstate, "SRAM_BANK1, ");
    if (pstate_bitset_none_set(pstate)) strcat(powman_last_pstate, "NONE, ");

    pstate_bitset_t default_pstate = pstate_bitset_none();
    low_power_persistent_pstate_get(&default_pstate);
    for (int i = 0; i < POWMAN_POWER_DOMAIN_COUNT; i++) {
        if (pstate_bitset_is_set(&default_pstate, i) && !pstate_bitset_is_set(pstate, i)) {
            strcat(powman_last_pstate, "PERSISTENT_DATA_OFF, ");
            if (my_number == 0) my_number = 34567;  // initialise my_number to special value
            break;
        }
    }
}
#endif

int main() {
    stdio_init_all();
    status_led_init();
    printf("Hello Sleep!\n");

    init_external_gpios();

    // use a repeating timer on the same TIMER instance; it should be disabled
    // during exclusive sleep (todo not sure how it affects power!)
    repeating_timer_t repeat;
    uint32_t repeater_id = 0;
    add_repeating_timer_ms(500, repeater, &repeater_id, &repeat);

    // test stdio_set_chars_available_callback
    stdio_set_chars_available_callback(chars_available_callback, NULL);

#if HAS_POWMAN_TIMER
    // use a second repeating timer on the other TIMER instance; it should be gated
    // during our sleep (todo not sure how it affects power!)
    alarm_pool_t *alarm_pool = alarm_pool_create_on_timer_with_unused_hardware_alarm(timer1_hw, 4);
    repeating_timer_t repeat2;
    uint32_t repeater2_id = 1;
    alarm_pool_add_repeating_timer_ms(alarm_pool, 700, repeater, &repeater2_id, &repeat2);

    if (my_number == 0) {
        // initialise persistent data
        my_number = 12345;
        memset(large_thing, 0x55, sizeof(large_thing));
        // track number of reboots
        powman_hw->scratch[3] = 0;
    }

    if (came_from_pstate) {
        printf("Came from powerup %s with (%s) memory kept on - skipping to end\n", powman_last_pwrup, powman_last_pstate);
        if (strstr(powman_last_pstate, "NONE") != NULL) {
            goto post_pstate_sram_off;
        } else {
            goto post_pstate_sram_on;
        }
    }

    pstate_bitset_t pstate;
#endif

    printf("Waiting %d seconds\n", SLEEP_TIME_S); // so we can see some repeat printfs
    busy_wait_ms(SLEEP_TIME_MS);

    absolute_time_t start_time;
    static absolute_time_t __persistent_data(wakeup_time);
    absolute_time_t system_time_before;
    int64_t diff;
    int ret;

    low_power_set_external_clock_source(DORMANT_CLOCK_HZ_DEFAULT, RTC_GPIO_IN);



    // exclusive sleep
    printf("Going to sleep for %d seconds via TIMER\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_sleep_until_timer(timer_hw, wakeup_time, NULL, true);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_sleep_until_timer\n", ret);
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        EXIT_TEST;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // non-exclusive sleep
    printf("Going to non-exclusive sleep for %d seconds via TIMER\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_sleep_until_timer(timer_hw, wakeup_time, NULL, false);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_sleep_until_timer\n", ret);
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    diff = absolute_time_diff_us(wakeup_time, get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        EXIT_TEST;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // start the AON timer
    low_power_start_aon_timer_at_time_ms(0);
    printf("AON timer started @%dms\n", to_ms_since_boot(aon_timer_get_absolute_time()));



    // exclusive sleep using the AON timer
    printf("Going to sleep for %d seconds via AON timer\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = aon_timer_get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_sleep_until_aon_timer(wakeup_time, NULL, true);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_sleep_until_aon_timer\n", ret);
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        EXIT_TEST;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // non-exclusive sleep using the AON timer
    printf("Going to non-exclusive sleep for %d seconds via AON timer\n", SLEEP_TIME_S);

    // leave the system timer running 
    clock_dest_bitset_t keep_enabled = clock_dest_bitset_none();
#if PICO_RP2040
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_SYS_TIMER);
#else
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_SYS_TIMER0);
    clock_dest_bitset_add(&keep_enabled, CLK_DEST_REF_TICKS);
#endif

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = aon_timer_get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_sleep_until_aon_timer(wakeup_time, &keep_enabled, false);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_sleep_until_aon_timer\n", ret);
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("ERROR: Woke up too soon\n");
        EXIT_TEST;
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // dormant using default clock source
    printf("Going DORMANT for %d seconds via AON TIMER\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = aon_timer_get_absolute_time();
    system_time_before = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_dormant_until_aon_timer(wakeup_time, DORMANT_CLOCK_SOURCE_DEFAULT, NULL);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_dormant_until_aon_timer\n", ret);
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    // check the system timer was stopped while dormant
    diff = absolute_time_diff_us(system_time_before, get_absolute_time());
    if (diff > 200 * 1000 // 200ms
        #ifdef PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS
        + (PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS * 1000)
        #endif
    ) {
        printf("ERROR: doesn't seem like timer was stopped: diff %lldus\n", diff);
        return - 1;
    }
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // powman states
#if HAS_POWMAN_TIMER
    // pstate with sram0 on
    printf("Going to PSTATE with persistent data on for %d seconds\n", SLEEP_TIME_S);

    // Setup ext_ctrl0 to output on the SLEEP_MONITOR_PIN
    init_powman_ext_ctrl();

    if (my_number != 12345) {
        printf("ERROR: my_number is %d not 12345 - initialisation issue?\n", my_number);
        EXIT_TEST;
    }
    my_number = 67890;

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_set_pins_low_leakage_exclude_mask(USED_PIN_MASK);
    start_time = aon_timer_get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_pstate_until_aon_timer(wakeup_time, NULL, pstate_resume_func);

    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_pstate_until_aon_timer\n", ret);
        EXIT_TEST;
    }
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_sram_on:
    // track number of reboots
    powman_hw->scratch[3]++;
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    } else if (diff > 1000000
        #ifdef PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS
        + (PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS * 1000)
        #endif
    ) {
        printf("ERROR: Woke up more than %d seconds late\n", (int)(diff / 1000000));
        EXIT_TEST;
    }

    if (my_number != 67890) {
        printf("ERROR: my_number is %d not 67890 - SRAM has been re-loaded\n", my_number);
        EXIT_TEST;
    } else {
        printf("my_number in sram: %d\n", my_number);
    }

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);



    // pstate with sram off
    printf("Going to PSTATE with SRAM off for %d seconds\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    low_power_set_pins_low_leakage_exclude_mask(USED_PIN_MASK);
    start_time = aon_timer_get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    // store in scratch, as not persisting memory over this reboot
    powman_hw->scratch[0] = to_us_since_boot(wakeup_time) & 0xFFFFFFFF;
    powman_hw->scratch[1] = to_us_since_boot(wakeup_time) >> 32;
    pstate = pstate_bitset_none();
    ret = low_power_pstate_until_aon_timer(wakeup_time, &pstate, pstate_resume_func);

    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_pstate_until_aon_timer\n", ret);
        EXIT_TEST;
    }
    while (true) {
        printf("Waiting\n");
        busy_wait_ms(1000);
    }

post_pstate_sram_off:
    // track number of reboots
    powman_hw->scratch[3]++;
    // restore from scratch
    wakeup_time = from_us_since_boot((uint64_t)powman_hw->scratch[1] << 32 | (uint64_t)powman_hw->scratch[0]);
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    } else if (diff > 1000000
        #ifdef PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS
        + (PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS * 1000)
        #endif
    ) {
        printf("ERROR: Woke up more than %d seconds late\n", (int)(diff / 1000000));
        EXIT_TEST;
    }

    if (my_number != 34567) {
        printf("ERROR: my_number is %d not 34567 - SRAM has not been re-loaded\n", my_number);
        EXIT_TEST;
    } else {
        printf("my_number in sram: %d\n", my_number);
    }

    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);

    if (powman_hw->scratch[3] != 2) {
        printf("ERROR: number of POWMAN reboots was %d not 2\n", powman_hw->scratch[3]);
        EXIT_TEST;
    }
#else
    // dormant using XOSC clock source
    printf("Going DORMANT from the XOSC for %d seconds via AON TIMER\n", SLEEP_TIME_S);

    gpio_put(SLEEP_MONITOR_PIN, 0);
    start_time = aon_timer_get_absolute_time();
    system_time_before = get_absolute_time();
    wakeup_time = delayed_by_ms(start_time, SLEEP_TIME_MS);
    ret = low_power_dormant_until_aon_timer(wakeup_time, DORMANT_CLOCK_SOURCE_XOSC, NULL);
    if (ret != PICO_OK) {
        printf("ERROR: %d returned by low_power_dormant_until_aon_timer\n", ret);
    #if PICO_RP2040
        if (ret == PICO_ERROR_PRECONDITION_NOT_MET) {
            printf("ERROR: RTC clock source is not running - connect a device running external_sleep_timer to GPIO %d\n", RTC_GPIO_IN);
        }
    #endif
        EXIT_TEST;
    }
    gpio_put(SLEEP_MONITOR_PIN, 1);
    // check the system timer was stopped while dormant
    diff = absolute_time_diff_us(system_time_before, get_absolute_time());
    if (diff > 200 * 1000 // 200ms
        #ifdef PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS
        + (PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS * 1000)
        #endif
    ) {
        printf("ERROR: doesn't seem like timer was stopped: diff %lldus\n", diff);
        return - 1;
    }
    diff = absolute_time_diff_us(wakeup_time, aon_timer_get_absolute_time());
    printf("Woken up now @%dus since target\n", (int)diff);
    if (diff < 0) {
        printf("WARNING: Woke up too soon - is this within the resolution of the aon timer?\n");
    }
    printf("Doing %d second pause to prove timer running\n", SLEEP_TIME_S);
    busy_wait_ms(SLEEP_TIME_MS);
#endif

    printf("PASSED\n");

    return 0;
}