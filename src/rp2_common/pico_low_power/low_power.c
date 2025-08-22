/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"

#include "pico/low_power.h"
#include "pico/aon_timer.h"
#include "pico/runtime_init.h"

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

// todo this is a hack for now as we use setup_default_uart(); this is deliberately not in
//  the library dependencies right now, as this should get fixed up by better "pre/post" clock
//  setup hooks
#include "pico/stdlib.h"

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


// The difference between sleep and dormant is that ALL clocks are stopped in dormant mode,
// until the source (either xosc or rosc) is started again by an external event.
// In sleep mode some clocks can be left running controlled by the SLEEP_EN registers in the clocks
// block. For example you could keep clk_rtc running. Some destinations (proc0 and proc1 wakeup logic)
// can't be stopped in sleep mode otherwise there wouldn't be enough logic to wake up again.


// TODO: Optionally, memories can also be powered down.

// ------------------------------------------------------------------------------------------------------
// todo these probably belong in h/w clocks as some sort of registered thing, but leave them private here
//      for now
static void prepare_for_clock_gating(void) {
    // particularly for UART we want nothing left to clock out
    stdio_flush();
}

static void post_clock_gating(void) {
    // restore all clocks in sleep mode, to prevent other __wfi from causing issues
    clock_dest_set_t all = clock_dest_set_all();
    clock_gate_sleep_en(&all);
}

static uint32_t interrupt_flags;

static void prepare_for_clock_switch(void) {
    // particularly for UART we want nothing left to clock out
    prepare_for_clock_gating();

    // disable interrupts
    interrupt_flags = save_and_disable_interrupts();
}

static void post_clock_switch(void) {
    // restore UART baudrate
    setup_default_uart();

    // restore interrupts
    restore_interrupts_from_disabled(interrupt_flags);
}

#if HAS_POWMAN_TIMER
static void prepare_for_pstate_change(void) {
    prepare_for_clock_switch();
}

static void post_pstate_change(void) {
}
#endif

// ------------------------------------------------------------------------------------------------------

// todo should we make this a save/restore thing?
void low_power_enable_processor_deep_sleep(void) {
    // Enable deep sleep at the proc
#ifdef __riscv
    uint32_t bits = RVCSR_MSLEEP_POWERDOWN_BITS;
    if (!get_core_num()) {
        // todo errata ref
        bits |= RVCSR_MSLEEP_DEEPSLEEP_BITS;
    }
    riscv_set_csr(RVCSR_MSLEEP_OFFSET, bits);
#else
    scb_hw->scr |= ARM_CPU_PREFIXED(SCR_SLEEPDEEP_BITS);
#endif
}

void low_power_disable_processor_deep_sleep(void) {
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

static void replace_null_enable_values(const clock_dest_set_t *keep_enabled,
                                       clock_dest_set_t *local_keep_enabled) {
    if (keep_enabled) {
        *local_keep_enabled = *keep_enabled;
    } else {
        // default to keep nothing on
        *local_keep_enabled = clock_dest_set_none();
    }
}

// only the deep_sleep variant of this, as DORMANT cannot wake from TIMER
int low_power_sleep_until_timer(timer_hw_t *timer, absolute_time_t until,
                                const clock_dest_set_t *keep_enabled, __unused bool exclusive) {
    int alarm_num = timer_hardware_alarm_claim_unused(timer, false);
    if (alarm_num < 0) return PICO_ERROR_INSUFFICIENT_RESOURCES;

    event_happened = false;
    timer_hardware_alarm_set_callback(timer, alarm_num, ((hardware_alarm_callback_t )low_power_wakeup));
    if (timer_hardware_alarm_set_target(timer, alarm_num, until)) {
        timer_hardware_alarm_unclaim(timer, alarm_num);
        // the time has passed already
        return 0;
    }

    clock_dest_set_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);
    // todo we need mapping of hardware to clocks; also this needs to come from AON timer
    //  we know that people in the wild (MicroPython) have wanted to do some mapping to also
    //  figure out what PLLs are still on via these bits
#if PICO_RP2040
    clock_dest_set_add(&local_keep_enabled, CLK_DEST_SYS_TIMER);
#elif PICO_RP2350
    clock_dest_set_add(&local_keep_enabled, timer_get_index(timer) ? CLK_DEST_SYS_TIMER1 : CLK_DEST_SYS_TIMER0);
    clock_dest_set_add(&local_keep_enabled, CLK_DEST_REF_TICKS);
#else
#error Unknown processor
#endif
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

    return 0;
}

void low_power_sleep_until_pin_state(uint gpio_pin, bool edge, bool high,
                                     const clock_dest_set_t *keep_enabled, __unused bool exclusive) {

    event_happened = false;

    clock_dest_set_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    bool low = !high;
    bool level = !edge;

    // Configure the appropriate IRQ at IO bank 0
    assert(gpio_pin < NUM_BANK0_GPIOS);

    uint32_t event = 0;

    if (level && low) event = GPIO_IRQ_LEVEL_LOW;
    if (level && high) event = GPIO_IRQ_LEVEL_HIGH;
    if (edge && high) event = GPIO_IRQ_EDGE_RISE;
    if (edge && low) event = GPIO_IRQ_EDGE_FALL;

    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_irq_enabled_with_callback(gpio_pin, event, true, low_power_wakeup_gpio);

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
}

#if 0
// todo note this has (not surprisingly a lot of commonality with the timer one)
int low_power_sleep_until_aon_timer(absolute_time_t until,
                                    const clock_dest_set_t *keep_enabled) {
    event_happened = false;

    clock_dest_set_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);
    // Turn off all clocks except for the timer
    // todo we need mapping of hardware to clocks; also this needs to come from AON timer
    //  we know that people in the wild (MicroPython) have wanted to do some mapping to also
    //  figure out what PLLs are still on via these bits
    //
    // todo note also here that this should come from AON_TIMER via AON_TIMER_CLOCK_DEST_NUM() but
    //  how do we indicate multiple bits there; actually todo (graham) and no one steal this as it sounds
    //  fun, we probably want to have a "sparse" bitset macro encoded as 4 bytes or 8 bytes (for 4 or 7 indices
    //  between 0 and 254) - i say 7, to leave encoding space for maybe indices > 256 in the 8 byte variant
#if PICO_RP2040
    clock_dest_set_add(&local_keep_enabled, CLOCK_DEST_RTC_RTC);
#elif PICO_RP2350
    clock_dest_set_add(&local_keep_enabled, CLK_DEST_REF_POWMAN);
#else
#error Unknown processor
#endif
    // todo catch race condition here (or just plain in the past)
    struct timespec ts;
    us_to_timespec(to_us_since_boot(until), &ts);
    // note wakeup from low power == false, means don't wake up from dormant
    aon_timer_enable_alarm(&ts, (aon_timer_alarm_handler_t)low_power_wakeup, false);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();
    // Go to sleep until the wakeup event happens (note it may have happened already)
    while (!event_happened) {
        __wfi();
    }
    low_power_disable_processor_deep_sleep();

    post_clock_gating();
    return 0;
}
#endif

// In order to go into dormant mode we need to be running from a stoppable clock source:
// either the xosc or rosc with no PLLs running. This means we disable the USB and ADC clocks
// and all PLLs
void low_power_setup_clocks_for_dormant(dormant_clock_source_t dormant_source) {
    prepare_for_clock_switch();

    uint src_hz;
    uint32_t clk_ref_src;
    switch (dormant_source) {
        case DORMANT_CLOCK_SOURCE_XOSC:
            src_hz = XOSC_HZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC;
            break;
        case DORMANT_CLOCK_SOURCE_ROSC:
            src_hz = 6500 * KHZ; // todo
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH;
            break;
#if !PICO_RP2040
        case DORMANT_CLOCK_SOURCE_LPOSC:
            src_hz = 32 * KHZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_LPOSC_CLKSRC;
            break;
#endif
        default:
            hard_assert(false);
            __builtin_unreachable();
    }

    clock_configure_undivided(clk_ref,
                              clk_ref_src,
                              0,
                              src_hz);

    // CLK SYS = CLK_REF
    clock_configure_undivided(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0, // Using glitchless mux
                    src_hz);


    // CLK ADC = 0MHz
    clock_stop(clk_adc);
    clock_stop(clk_usb);
#if HAS_HSTX
    clock_stop(clk_hstx);
#endif

#if HAS_RP2040_RTC
    // CLK RTC = ideally XOSC (12MHz) / 256 = 46875Hz but could be rosc
    uint clk_rtc_src = (dormant_source == DORMANT_CLOCK_SOURCE_XOSC) ?
                       CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC :
                       CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;

    clock_configure(clk_rtc,
                    0, // No GLMUX
                    clk_rtc_src,
                    src_hz,
                    46875);
#endif

    // CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    src_hz,
                    src_hz);

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
void low_power_wake_from_dormant(void) {
    //Re-enable the ring oscillator, which will essentially kickstart the proc
    rosc_restart();

    post_clock_gating();

    //Restore all inactive clocks
    runtime_init_clocks();
    post_clock_switch();
}

void low_power_go_dormant(dormant_clock_source_t dormant_clock_source) {
    assert(dormant_clock_source == DORMANT_CLOCK_SOURCE_XOSC || dormant_clock_source == DORMANT_CLOCK_SOURCE_ROSC);

    if (dormant_clock_source == DORMANT_CLOCK_SOURCE_XOSC) {
        xosc_dormant();
    } else {
        rosc_set_dormant();
    }
}

int low_power_dormant_until_aon_timer(absolute_time_t until,
                                      dormant_clock_source_t dormant_clock_source,
                                      uint src_hz, uint gpio_pin,
                                      const clock_dest_set_t *keep_enabled) {
    low_power_setup_clocks_for_dormant(dormant_clock_source);

    clock_dest_set_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    // todo ugh this doesn't really belong here like this; need to encapsulate in aon timer?
#if PICO_RP2040
    // The RTC must be run from an external source, since the dormant source will be inactive
    rtc_run_from_external_source(src_hz, gpio_pin);
    clock_dest_set_add(&local_keep_enabled, CLK_DEST_RTC_RTC);
#elif PICO_RP2350
    // todo
    ((void)src_hz);
    ((void)gpio_pin);
    if (dormant_clock_source == DORMANT_CLOCK_SOURCE_LPOSC)
        powman_timer_set_1khz_tick_source_lposc();
    else
        return PICO_ERROR_INVALID_ARG;

    clock_dest_set_add(&local_keep_enabled, CLK_DEST_REF_POWMAN);
#else
    #error Unknown processor
#endif

    // todo catch race condition here (or just plain in the past)
    struct timespec ts;
    us_to_timespec(to_us_since_boot(until), &ts);
    event_happened = false;
    // note wakeup from low power == false, means don't wake up from dormant
    aon_timer_enable_alarm(&ts, (aon_timer_alarm_handler_t)low_power_wakeup, true);

    prepare_for_clock_gating();
    // gate clocks
    clock_gate_sleep_en(&local_keep_enabled);

    low_power_enable_processor_deep_sleep();

    //Go dormant
    low_power_go_dormant(dormant_clock_source);


    assert(event_happened); // does it?
    low_power_wake_from_dormant();

    return 0;
}

void low_power_dormant_until_pin_state(uint gpio_pin, bool edge, bool high,
                                       dormant_clock_source_t dormant_clock_source,
                                       const clock_dest_set_t *keep_enabled) {

    low_power_setup_clocks_for_dormant(dormant_clock_source);

    clock_dest_set_t local_keep_enabled;
    replace_null_enable_values(keep_enabled, &local_keep_enabled);

    bool low = !high;
    bool level = !edge;

    // Configure the appropriate IRQ at IO bank 0
    assert(gpio_pin < NUM_BANK0_GPIOS);

    uint32_t event = 0;

    if (level && low) event = GPIO_IRQ_LEVEL_LOW;
    if (level && high) event = GPIO_IRQ_LEVEL_HIGH;
    if (edge && high) event = GPIO_IRQ_EDGE_RISE;
    if (edge && low) event = GPIO_IRQ_EDGE_FALL;

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
}

#if !PICO_RP2040
int low_power_pstate_set(pstate_bitset_t *pstate) {
    invalid_params_if(PICO_LOW_POWER, !pstate_bitset_is_set(pstate, POWMAN_POWER_DOMAIN_SWITCHED_CORE));

    return powman_set_power_state(pstate_bitset_to_powman_power_state(pstate));
}

pstate_bitset_t low_power_pstate_get(void) {
    return pstate_bitset_from_powman_power_state(powman_get_power_state());
}

int low_power_go_pstate(pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    prepare_for_pstate_change();

    // Configure the wakeup state
    pstate_bitset_t current_pstate = low_power_pstate_get();
    bool valid_state = powman_configure_wakeup_state(pstate_bitset_to_powman_power_state(pstate), pstate_bitset_to_powman_power_state(&current_pstate));
    if (!valid_state) {
        return PICO_ERROR_INVALID_STATE;
    }

    // reboot to main
    powman_hw->boot[0] = 0;
    powman_hw->boot[1] = 0;
    powman_hw->boot[2] = 0;
    powman_hw->boot[3] = 0;

    powman_hw->scratch[7] = (uint32_t)resume_func;

    // Switch to required power state
    int rc = powman_set_power_state(pstate_bitset_to_powman_power_state(pstate));
    if (rc != PICO_OK) {
        return rc;
    }

    // Power down
    while (true) __wfi();

    post_pstate_change();

    return rc;
}

int low_power_pstate_until_aon_timer(absolute_time_t until, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    powman_enable_alarm_wakeup_at_ms(to_ms_since_boot(until));

    return low_power_go_pstate(pstate, resume_func);
}

int low_power_pstate_until_pin_state(uint gpio_pin, bool edge, bool high, pstate_bitset_t *pstate, low_power_pstate_resume_func resume_func) {
    powman_enable_gpio_wakeup(0, gpio_pin, edge, high);

    return low_power_go_pstate(pstate, resume_func);
}

#if !PICO_RUNTIME_NO_INIT_LOW_POWER_REBOOT_CHECK
void __weak runtime_init_low_power_reboot_check(void) {
    // check if we came from powman reboot
    if (powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_SWCORE_PD_BITS) {
        // we came from powman reboot, so execute the resume function
        if (powman_hw->scratch[7]) {
            ((low_power_pstate_resume_func)powman_hw->scratch[7])();
            powman_hw->scratch[7] = 0;
        }
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_LOW_POWER_REBOOT_CHECK
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_low_power_reboot_check, PICO_RUNTIME_INIT_LOW_POWER_REBOOT_CHECK);
#endif

#endif // !PICO_RP2040

#if !PICO_RUNTIME_NO_INIT_RP2350_SLEEP_FIX
#include "hardware/sync.h"
void __weak runtime_init_rp2350_sleep_fix(void) {
    if (watchdog_hw->reason && WATCHDOG_REASON_TIMER_BITS) { // detect rom_reboot() usage
        int alarm_num = timer_hardware_alarm_claim_unused(timer_hw, false);
        if (alarm_num < 0) return;

        timer_hardware_alarm_set_callback(timer_hw, alarm_num, ((hardware_alarm_callback_t )low_power_wakeup));
        timer_hardware_alarm_set_target(timer_hw, alarm_num, make_timeout_time_us(100));

        __wfi();

        timer_hardware_alarm_set_callback(timer_hw, alarm_num, NULL);
        timer_hardware_alarm_unclaim(timer_hw, alarm_num);
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_RP2350_SLEEP_FIX
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_rp2350_sleep_fix, PICO_RUNTIME_INIT_RP2350_SLEEP_FIX);
#endif
