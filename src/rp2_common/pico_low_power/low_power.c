/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico.h"

#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/runtime_init.h"
#include "pico/time.h"
#include "pico/stdio.h"
#if LIB_PICO_STDIO_USB
#include "pico/stdio_usb.h"
#endif

#include "hardware/pll.h"
#include "hardware/claim.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/rosc.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"

#if LIB_TINYUSB_DEVICE || LIB_TINYUSB_HOST
#include "tusb.h"
#endif

#if HAS_RP2040_RTC
#include "hardware/rtc.h"
#elif HAS_POWMAN_TIMER
#include "hardware/powman.h"
#endif

// For __wfi
#ifdef __riscv
#include "hardware/riscv.h"
#else
// For scb_hw so we can enable deep sleep
#include "hardware/structs/scb.h"
#endif

// ------------------------------------------------------------------------------------------------------
// todo these probably belong in h/w clocks as some sort of registered thing, but leave them private here
//      for now
static void prepare_for_clock_gating(void) {
    // particularly for UART we want nothing left to clock out
    stdio_flush();
}

static void post_clock_gating(void) {
    // restore all clocks in sleep mode, to prevent other __wfi from causing issues
    clock_dest_bitset_t all = clock_dest_bitset_all();
    clock_gate_sleep_en(&all);
}

static bool is_timestamp_from_aon_timer(absolute_time_t timestamp) {
    return to_us_since_boot(timestamp) % 1000 == 0;
}

static uint32_t interrupt_flags;

#if LIB_TINYUSB_DEVICE
static bool tud_was_inited = false;
#endif

#if LIB_TINYUSB_HOST
static bool tuh_was_inited = false;
#endif

static void prepare_for_clock_switch(void) {
    // particularly for UART we want nothing left to clock out
    prepare_for_clock_gating();

#if LIB_TINYUSB_DEVICE
    tud_was_inited = tud_inited();
    if (tud_was_inited) tud_deinit(0);
#endif

#if LIB_TINYUSB_HOST
    tuh_was_inited = tuh_inited();
    if (tuh_was_inited) tuh_deinit(0);
#endif

#if LIB_PICO_STDIO_USB
    // deinit USB
    stdio_usb_deinit();
#endif

    // disable interrupts
    interrupt_flags = save_and_disable_interrupts();
}

static void post_clock_switch(void) {
    // restore interrupts
    restore_interrupts_from_disabled(interrupt_flags);

#if LIB_TINYUSB_DEVICE
    if (tud_was_inited) tud_init(0);
#endif

#if LIB_TINYUSB_HOST
    if (tuh_was_inited) tuh_init(0);
#endif

#if LIB_PICO_STDIO_USB
    // reinit USB
    stdio_usb_init();
#endif
}

#if HAS_POWMAN_TIMER
static void prepare_for_pstate_change(void) {
    prepare_for_clock_switch();
    // Ensure debugger does not prevent power down
    powman_set_debug_power_request_ignored(true);
    // Switch powman timer to lposc explicitly, which will also use the calibrated frequency
    powman_timer_set_1khz_tick_source_lposc();
}

static void post_pstate_change(void) {
}
#endif

static void low_power_enable_processor_deep_sleep(void) {
    // Enable deep sleep at the proc
#ifdef __riscv
    uint32_t bits = RVCSR_MSLEEP_POWERDOWN_BITS;
    if (!get_core_num()) {
        // see errata RP2350-E4
        bits |= RVCSR_MSLEEP_DEEPSLEEP_BITS;
    }
    riscv_set_csr(RVCSR_MSLEEP_OFFSET, bits);
#else
    scb_hw->scr |= ARM_CPU_PREFIXED(SCR_SLEEPDEEP_BITS);
#endif
}

static void low_power_disable_processor_deep_sleep(void) {
#ifdef __riscv
    riscv_clear_csr(RVCSR_MSLEEP_OFFSET, RVCSR_MSLEEP_POWERDOWN_BITS | RVCSR_MSLEEP_DEEPSLEEP_BITS);
#else
    scb_hw->scr &= ~ARM_CPU_PREFIXED(SCR_SLEEPDEEP_BITS);
#endif
}

volatile bool event_happened;

static void low_power_wakeup(void) {
    event_happened = true;
}

static void low_power_wakeup_gpio(__unused uint gpio, __unused uint32_t event_mask) {
    low_power_wakeup();
}

static void replace_null_enable_values(const clock_dest_bitset_t *keep_enabled,
                                       clock_dest_bitset_t *local_keep_enabled) {
    if (keep_enabled) {
        *local_keep_enabled = *keep_enabled;
    } else {
        // default to keep nothing on
        *local_keep_enabled = clock_dest_bitset_none();
    }
}

static void add_library_clocks(clock_dest_bitset_t *local_keep_enabled) {
#if LIB_PICO_STDIO_USB || LIB_TINYUSB_HOST || LIB_TINYUSB_DEVICE
    // this is necessary to prevent dropping the connection
    #if PICO_RP2040
        clock_dest_bitset_add(local_keep_enabled, CLK_DEST_SYS_USBCTRL);
        clock_dest_bitset_add(local_keep_enabled, CLK_DEST_USB_USBCTRL);
    #elif PICO_RP2350
        clock_dest_bitset_add(local_keep_enabled, CLK_DEST_SYS_USBCTRL);
        clock_dest_bitset_add(local_keep_enabled, CLK_DEST_USB);
    #else
    #error Unknown processor
    #endif
#endif

#if LIB_PICO_STDIO_UART
    // this is only needed to prevent losing stdin while sleeping
    clock_dest_bitset_add(local_keep_enabled, PICO_DEFAULT_UART ? CLK_DEST_PERI_UART1 : CLK_DEST_PERI_UART0);
    clock_dest_bitset_add(local_keep_enabled, PICO_DEFAULT_UART ? CLK_DEST_SYS_UART1 : CLK_DEST_SYS_UART0);
#endif
}

// Ceiling division of NUM_IRQS by 32
#define NUM_IRQ_WORDS ((NUM_IRQS / 32) + ((NUM_IRQS % 32) > 0))

static uint32_t irq_mask_disabled_during_sleep[NUM_IRQ_WORDS];

static void save_and_disable_other_interrupts(uint32_t irq) {
    for (uint n = 0; n < NUM_IRQ_WORDS; n++) {
        irq_mask_disabled_during_sleep[n] = irq_get_mask_n(n);
        if (irq >= n * 32 && irq < (n + 1) * 32) {
            irq_mask_disabled_during_sleep[n] &= ~(1u << (irq % 32));
        }
        irq_set_mask_n_enabled(n, irq_mask_disabled_during_sleep[n], false);
    }
}

static void restore_other_interrupts(void) {
    for (uint n = 0; n < NUM_IRQ_WORDS; n++) {
        irq_set_mask_n_enabled(n, irq_mask_disabled_during_sleep[n], true);
    }
}

#if PICO_RP2040
static uint dormant_rtc_src_hz = 0;
static uint dormant_rtc_gpio_pin;
int low_power_set_external_clock_source(uint src_hz, uint gpio_pin) {
    dormant_rtc_src_hz = src_hz;
    dormant_rtc_gpio_pin = gpio_pin;
    return PICO_OK;
}
#endif  // inline in header for other platforms

int low_power_sleep_until_irq(const clock_dest_bitset_t *keep_enabled) {
    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    add_library_clocks(&local_keep_enabled);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();
    // Go to sleep until any event happens
    __wfi();
    low_power_disable_processor_deep_sleep();

    post_clock_gating();

    return 0;
}

// only the deep_sleep variant of this, as DORMANT cannot wake from TIMER
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until,
                                const clock_dest_bitset_t *keep_enabled, bool exclusive) {
    int alarm_num = timer_hardware_alarm_claim_unused(timer, false);
    if (alarm_num < 0) return PICO_ERROR_INSUFFICIENT_RESOURCES;

    event_happened = false;
    timer_hardware_alarm_set_callback(timer, alarm_num, ((hardware_alarm_callback_t )low_power_wakeup));
    if (timer_hardware_alarm_set_target(timer, alarm_num, until)) {
        timer_hardware_alarm_unclaim(timer, alarm_num);
        // the time has passed already
        return 0;
    }

    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);
#if PICO_RP2040
    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_SYS_TIMER);
#elif PICO_RP2350
    clock_dest_bitset_add(&local_keep_enabled, timer_get_index(timer) ? CLK_DEST_SYS_TIMER1 : CLK_DEST_SYS_TIMER0);
    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_REF_TICKS);
#else
#error Unknown processor
#endif

    add_library_clocks(&local_keep_enabled);

#if NUM_GENERIC_TIMERS == 1
#define TIMER_BASE_IRQ TIMER_IRQ_0
#else
#define TIMER_BASE_IRQ TIMER0_IRQ_0
#endif

    if (exclusive) save_and_disable_other_interrupts(TIMER_BASE_IRQ + alarm_num + (timer_get_index(timer) * NUM_ALARMS));

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();
    // Go to sleep until the wakeup event happens (note it may have happened already)
    while (!event_happened) __wfi();
    low_power_disable_processor_deep_sleep();

    timer_hardware_alarm_set_callback(timer, alarm_num, NULL);
    timer_hardware_alarm_unclaim(timer, alarm_num);

    post_clock_gating();

    if (exclusive) restore_other_interrupts();

    return 0;
}

int low_power_sleep_until_aon_timer(absolute_time_t until,
                                    const clock_dest_bitset_t *keep_enabled, bool exclusive) {
    if (!aon_timer_is_running()) {
        return PICO_ERROR_PRECONDITION_NOT_MET;
    }

    if (!is_timestamp_from_aon_timer(until)) {
        return PICO_ERROR_INVALID_DATA;
    }

    if (to_ms_64_since_boot(aon_timer_get_absolute_time()) + PICO_LOW_POWER_MIN_AON_SLEEP_TIME_MS > to_ms_64_since_boot(until)) {
        // Prevent race condition where the timer fires before we can go to sleep
        // by setting a minimum time for sleep
        return PICO_ERROR_INVALID_ARG;
    }

    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

#if PICO_RP2040
    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_RTC_RTC);
#elif PICO_RP2350
    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_REF_POWMAN);
#else
    #error Unknown processor
#endif

    add_library_clocks(&local_keep_enabled);

    struct timespec ts;
    us_to_timespec(to_us_since_boot(until), &ts);
    event_happened = false;
    aon_timer_enable_alarm(&ts, low_power_wakeup, false);

    if (exclusive) save_and_disable_other_interrupts(aon_timer_get_irq_num());

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();
    // Go to sleep until the wakeup event happens (note it may have happened already)
    while (!event_happened) __wfi();
    low_power_disable_processor_deep_sleep();

    aon_timer_disable_alarm();

    post_clock_gating();

    if (exclusive) restore_other_interrupts();

    return 0;
}

int low_power_sleep_until_gpio_pin_state(uint gpio_pin, bool edge, bool high,
                                     const clock_dest_bitset_t *keep_enabled, bool exclusive) {

    invalid_params_if_and_return(PICO_LOW_POWER, gpio_pin >= NUM_BANK0_GPIOS, PICO_ERROR_INVALID_ARG);
    event_happened = false;

    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    add_library_clocks(&local_keep_enabled);

    // Configure the appropriate IRQ at IO bank 0

    uint32_t event = 0;

    if (edge) {
        event = high ? GPIO_IRQ_EDGE_RISE : GPIO_IRQ_EDGE_FALL;
    } else { // level
        event = high ? GPIO_IRQ_LEVEL_HIGH : GPIO_IRQ_LEVEL_LOW;
    }

    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_irq_enabled_with_callback(gpio_pin, event, true, low_power_wakeup_gpio);

    if (exclusive) save_and_disable_other_interrupts(IO_IRQ_BANK0);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();
    // Go to sleep until the wakeup event happens (note it may have happened already)
    while (!event_happened) __wfi();
    low_power_disable_processor_deep_sleep();

    // Clear the irq so we can go back to dormant mode again if we want
    gpio_acknowledge_irq(gpio_pin, event);
    gpio_set_irq_enabled_with_callback(gpio_pin, event, false, NULL);

    post_clock_gating();

    if (exclusive) restore_other_interrupts();

    return 0;
}

// In order to go into dormant mode we need to be running from a stoppable clock source:
// either the xosc or rosc with no PLLs running. This means we disable the USB and ADC clocks
// and all PLLs
static void low_power_setup_clocks_for_dormant(dormant_clock_source_t dormant_source) {
    prepare_for_clock_switch();

    uint clk_ref_src_hz;
    uint32_t clk_ref_src;
    uint clk_sys_src_hz;
    uint32_t clk_sys_src;
    uint32_t clk_sys_aux_src;
    switch (dormant_source) {
        case DORMANT_CLOCK_SOURCE_XOSC:
            clk_ref_src_hz = XOSC_HZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC;
            clk_sys_src_hz = clk_ref_src_hz;
            clk_sys_src = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF;
            clk_sys_aux_src = 0;
            break;
        case DORMANT_CLOCK_SOURCE_ROSC:
            clk_ref_src_hz = rosc_measure_freq_khz() * KHZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH;
            clk_sys_src_hz = clk_ref_src_hz;
            clk_sys_src = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF;
            clk_sys_aux_src = 0;
            break;
#if !PICO_RP2040
        case DORMANT_CLOCK_SOURCE_LPOSC:
            clk_ref_src_hz = 32 * KHZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_LPOSC_CLKSRC;
            clk_sys_src_hz = rosc_measure_freq_khz() * KHZ;
            clk_sys_src = CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX;
            clk_sys_aux_src = CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_ROSC_CLKSRC;
            break;
#endif
        default:
            hard_assert(false);
            __builtin_unreachable();
    }

    clock_configure_undivided(clk_ref,
                              clk_ref_src,
                              0,
                              clk_ref_src_hz);

    // CLK SYS = CLK_REF
    clock_configure_undivided(clk_sys,
                    clk_sys_src,
                    clk_sys_aux_src,
                    clk_sys_src_hz);


    // CLK ADC = 0MHz
    clock_stop(clk_adc);
    clock_stop(clk_usb);
#if HAS_HSTX
    clock_stop(clk_hstx);
#endif

#if HAS_RP2040_RTC
    // RTC should already be configured to run from the external source
#endif

    // CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    clk_sys_src_hz,
                    clk_sys_src_hz);

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    // Assuming both xosc and rosc are running at the moment
    if (dormant_source == DORMANT_CLOCK_SOURCE_XOSC) {
        // Safe to disable rosc
        rosc_disable();
    } else {
        // Safe to disable xosc
        xosc_disable();
    }
}

//To be called after waking up from sleep/dormant mode to restore system clocks properly
static void low_power_wake_from_dormant(void) {
    //Re-enable the ring oscillator, which will essentially kickstart the proc
    rosc_restart();

    post_clock_gating();

    //Restore all inactive clocks
    runtime_init_clocks();
    post_clock_switch();
}

static void low_power_go_dormant(dormant_clock_source_t dormant_clock_source) {
    invalid_params_if(PICO_LOW_POWER,
        dormant_clock_source == DORMANT_CLOCK_SOURCE_XOSC || dormant_clock_source == DORMANT_CLOCK_SOURCE_ROSC
    #if !PICO_RP2040
        || dormant_clock_source == DORMANT_CLOCK_SOURCE_LPOSC
    #endif
    );

    if (dormant_clock_source == DORMANT_CLOCK_SOURCE_XOSC) {
        xosc_dormant();
    } else {
        rosc_set_dormant();
    }
}

int low_power_dormant_until_aon_timer(absolute_time_t until,
                                      dormant_clock_source_t dormant_clock_source,
                                      const clock_dest_bitset_t *keep_enabled) {
    if (!aon_timer_is_running()) {
        return PICO_ERROR_PRECONDITION_NOT_MET;
    }

    if (!is_timestamp_from_aon_timer(until)) {
        return PICO_ERROR_INVALID_DATA;
    }

    if (to_ms_64_since_boot(aon_timer_get_absolute_time()) + PICO_LOW_POWER_MIN_DORMANT_TIME_MS > to_ms_64_since_boot(until)) {
        // Prevent race condition where the timer fires before we can go dormant
        // by setting a minimum time for dormant
        return PICO_ERROR_INVALID_ARG;
    }

    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

#if PICO_RP2040
    if (dormant_rtc_src_hz == 0) {
        return PICO_ERROR_PRECONDITION_NOT_MET;
    }
    // The RTC must be run from an external source, since the dormant source will be inactive
    if (!rtc_run_from_external_source(dormant_rtc_src_hz, dormant_rtc_gpio_pin)) {
        return PICO_ERROR_PRECONDITION_NOT_MET;
    }
    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_RTC_RTC);
#elif PICO_RP2350
    if (dormant_clock_source == DORMANT_CLOCK_SOURCE_LPOSC)
        powman_timer_set_1khz_tick_source_lposc();
    else
        return PICO_ERROR_INVALID_ARG;

    clock_dest_bitset_add(&local_keep_enabled, CLK_DEST_REF_POWMAN);
#else
    #error Unknown processor
#endif

    low_power_setup_clocks_for_dormant(dormant_clock_source);

    struct timespec ts;
    us_to_timespec(to_us_since_boot(until), &ts);
    event_happened = false;
    aon_timer_enable_alarm(&ts, NULL, true);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();

    //Go dormant
    low_power_go_dormant(dormant_clock_source);

    low_power_wake_from_dormant();

#if PICO_RP2350
    if (dormant_clock_source == DORMANT_CLOCK_SOURCE_LPOSC)
        powman_timer_set_1khz_tick_source_xosc();
#endif

    return 0;
}

int low_power_dormant_until_gpio_pin_state(uint gpio_pin, bool edge, bool high,
                                       dormant_clock_source_t dormant_clock_source,
                                       const clock_dest_bitset_t *keep_enabled) {

    invalid_params_if_and_return(PICO_LOW_POWER, gpio_pin >= NUM_BANK0_GPIOS, PICO_ERROR_INVALID_ARG);

    low_power_setup_clocks_for_dormant(dormant_clock_source);

    clock_dest_bitset_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    // Configure the appropriate IRQ at IO bank 0

    uint32_t event = 0;

    if (edge) {
        event = high ? GPIO_IRQ_EDGE_RISE : GPIO_IRQ_EDGE_FALL;
    } else { // level
        event = high ? GPIO_IRQ_LEVEL_HIGH : GPIO_IRQ_LEVEL_LOW;
    }

    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_dormant_irq_enabled(gpio_pin, event, true);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();

    //Go dormant
    low_power_go_dormant(dormant_clock_source);

    // Clear the irq so we can go back to dormant mode again if we want
    gpio_acknowledge_irq(gpio_pin, event);
    gpio_set_dormant_irq_enabled(gpio_pin, event, false);

    low_power_wake_from_dormant();

    return 0;
}

#if HAS_POWMAN_TIMER
extern unsigned char __persistent_data_start__[];
extern unsigned char __persistent_data_end__[];

static pstate_bitset_t *low_power_pstate_get(pstate_bitset_t *pstate) {
    pstate_bitset_from_powman_power_state(pstate, powman_get_power_state());
    return pstate;
}

pstate_bitset_t *low_power_persistent_pstate_get(pstate_bitset_t *pstate) {
    pstate_bitset_remove_all(pstate);

    if ((uint32_t)__persistent_data_start__ == (uint32_t)__persistent_data_end__) {
        // No persistent data, so power down everything
        return pstate;
    }

    // Keep __persistent_data_start__ on
    if ((uint32_t)__persistent_data_start__ < SRAM_BASE) {
        pstate_bitset_add(pstate, POWMAN_POWER_DOMAIN_XIP_CACHE);
    } else if ((uint32_t)__persistent_data_start__ < SRAM4_BASE) {
        pstate_bitset_add(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK0);

        // Keep __persistent_data_end__ on too, if it is in SRAM bank 1
        if ((uint32_t)__persistent_data_end__ >= SRAM4_BASE) {
            pstate_bitset_add(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK1);
        }
    } else {
        pstate_bitset_add(pstate, POWMAN_POWER_DOMAIN_SRAM_BANK1);
    }

    return pstate;
}

static int low_power_go_pstate(pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    pstate_bitset_t default_pstate = pstate_bitset_none();
    if (pstate == NULL) {
        pstate = &default_pstate;
        low_power_persistent_pstate_get(pstate);
    }

    prepare_for_pstate_change();

    // Configure the wakeup state
    pstate_bitset_t current_pstate = pstate_bitset_none();
    low_power_pstate_get(&current_pstate);
    bool valid_state = powman_configure_wakeup_state(pstate_bitset_to_powman_power_state(pstate), pstate_bitset_to_powman_power_state(&current_pstate));
    if (!valid_state) {
        return PICO_ERROR_INVALID_STATE;
    }

    // reboot to main
    powman_hw->boot[0] = 0;
    powman_hw->boot[1] = 0;
    powman_hw->boot[2] = 0;
    powman_hw->boot[3] = 0;

    // Store the low power state and resume function for use after reboot
    powman_hw->scratch[6] = pstate_bitset_to_powman_power_state(pstate);
    powman_hw->scratch[7] = (uint32_t)resume_func;

    // Switch to required power state
    int rc = powman_set_power_state(pstate_bitset_to_powman_power_state(pstate));
    if (rc != PICO_OK) {
        return rc;
    }

    // Power down
    while (true) __wfi();

    // Should not reach here
    post_pstate_change();

    return rc;
}

int low_power_pstate_until_aon_timer(absolute_time_t until, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    if (!aon_timer_is_running()) {
        return PICO_ERROR_PRECONDITION_NOT_MET;
    }

    if (!is_timestamp_from_aon_timer(until)) {
        return PICO_ERROR_INVALID_DATA;
    }

    if (to_ms_64_since_boot(aon_timer_get_absolute_time()) + PICO_LOW_POWER_MIN_PSTATE_TIME_MS > to_ms_64_since_boot(until)) {
        // Prevent race condition where the timer fires before we can go to pstate
        // by setting a minimum time for pstate
        return PICO_ERROR_INVALID_ARG;
    }
    powman_enable_alarm_wakeup_at_ms(to_ms_64_since_boot(until));

    return low_power_go_pstate(pstate, resume_func);
}

int low_power_pstate_until_gpio_pin_state(uint gpio_pin, bool edge, bool high, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    powman_enable_gpio_wakeup(0, gpio_pin, edge, high);

    return low_power_go_pstate(pstate, resume_func);
}

#if !PICO_RUNTIME_NO_INIT_LOW_POWER_REBOOT_CHECK

static inline void reset_persistent_data(void) {
    size_t persistent_data_size = (uint32_t)__persistent_data_end__ - (uint32_t)__persistent_data_start__;
    __builtin_memset((void*)(uint32_t)__persistent_data_start__, 0, persistent_data_size);
}

void __weak runtime_init_low_power_reboot_check(void) {
    // check if we came from powman reboot
    if (powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_SWCORE_PD_BITS) {
        pstate_bitset_t persistent_pstate = pstate_bitset_none();
        low_power_persistent_pstate_get(&persistent_pstate);
        pstate_bitset_t pstate = pstate_bitset_none();
        pstate_bitset_from_powman_power_state(&pstate, powman_hw->scratch[6]);

        // check if persistent data was turned off
        if (!pstate_bitset_none_set(&persistent_pstate)) {
            for (int i=0; i < POWMAN_POWER_DOMAIN_COUNT; i++) {
                if (!pstate_bitset_is_set(&pstate, (powman_power_domain_t)i) && pstate_bitset_is_set(&persistent_pstate, (powman_power_domain_t)i)) {
                    reset_persistent_data();
                    break;
                }
            }
        }

        // execute the resume function, if present
        if (powman_hw->scratch[7]) {
            ((low_power_pstate_resume_func)powman_hw->scratch[7])(&pstate);
        }

        // clear the scratch registers
        powman_hw->scratch[6] = 0;
        powman_hw->scratch[7] = 0;

        // Switch powman timer back to xosc
        powman_timer_set_1khz_tick_source_xosc();
    } else {
        // not a powman reboot, so clear persistent data
        reset_persistent_data();
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_LOW_POWER_REBOOT_CHECK
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_low_power_reboot_check, PICO_RUNTIME_INIT_LOW_POWER_REBOOT_CHECK);
#endif

#if !PICO_RUNTIME_NO_INIT_LOW_POWER_CACHE_UNPIN
void __weak __no_inline_not_in_flash_func(runtime_init_low_power_cache_unpin)(void) {
    // if persistent data is in xip_sram, then the whole cache is currently pinned
    // for performance, we should unpin the rest of it
    if ((uint32_t)__persistent_data_start__ < SRAM_BASE) {
        uint32_t persistent_data_start_maintenance = XIP_MAINTENANCE_BASE + ((uint32_t)__persistent_data_start__ - XIP_BASE);
        uint32_t persistent_data_end_maintenance = XIP_MAINTENANCE_BASE + ((uint32_t)__persistent_data_end__ - XIP_BASE);
        volatile uint8_t* cache;
        for (
            cache = (volatile uint8_t*)(XIP_MAINTENANCE_BASE + XIP_SRAM_BASE - XIP_BASE);
            cache < (volatile uint8_t*)(XIP_MAINTENANCE_BASE + XIP_END - XIP_BASE);
            cache += 8
        ) {
            if ((uint32_t)cache >= persistent_data_start_maintenance && (uint32_t)cache < persistent_data_end_maintenance) {
                continue;
            }
            *(cache + 0) = 0; // invalidate
        }
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_LOW_POWER_CACHE_UNPIN
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_low_power_cache_unpin, PICO_RUNTIME_INIT_LOW_POWER_CACHE_UNPIN);
#endif

#endif // HAS_POWMAN_TIMER

#if !PICO_RUNTIME_NO_INIT_RP2350_SLEEP_FIX
#include "hardware/sync.h"
void __weak __not_in_flash_func(runtime_init_rp2350_sleep_fix)(void) {
    if (watchdog_hw->reason && WATCHDOG_REASON_TIMER_BITS) { // detect rom_reboot() usage
        uint32_t flags = save_and_disable_interrupts();

        // Clear (and save) NVIC mask so only the dummy can fire
        uint32_t saved_irq_mask[NUM_IRQ_WORDS];
        for (uint i = 0; i < NUM_IRQ_WORDS; ++i) {
            saved_irq_mask[i] = nvic_hw->icer[i];
            nvic_hw->icer[i] = -1u;
        }

        // Un-pend then enable the dummy
        const uint32_t dummy_irq_idx = FIRST_USER_IRQ / 32u;
        const uint32_t dummy_irq_bit = FIRST_USER_IRQ % 32u;
        nvic_hw->icpr[dummy_irq_idx] = 1u << dummy_irq_bit;
        nvic_hw->iser[dummy_irq_idx] = 1u << dummy_irq_bit;

        // Sleep and immediately dummy-IRQ back out of sleep (these events happen
        // in reverse order on M33; Armv8-M doesn't specify the ordering)
        pico_default_asm_volatile (
            ".p2align 2\n"    // Make sure both 16-bit instructions are fetched
            "str %0, [%1]\n"
            "wfi\n"
            :
            : "l" (1u << dummy_irq_bit),
            "l" (&nvic_hw->ispr[dummy_irq_idx])
        );

        // Restore NVIC mask
        nvic_hw->icer[dummy_irq_idx] = 1u << dummy_irq_bit;
        for (uint i = 0; i < NUM_IRQ_WORDS; ++i) {
            nvic_hw->iser[i] = saved_irq_mask[i];
        }

        restore_interrupts(flags);
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_RP2350_SLEEP_FIX
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_rp2350_sleep_fix, PICO_RUNTIME_INIT_RP2350_SLEEP_FIX);
#endif
