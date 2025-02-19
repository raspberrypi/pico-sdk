/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"

// Doesn't make any sense for a RAM only binary
#if !PICO_NO_FLASH

#include "pico/time.h"
#include "pico/bootrom.h"
#include "pico/binary_info.h"

#if !PICO_RP2040
#include "hardware/structs/powman.h"
#endif

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS, Window of opportunity for a second press of a reset button to enter BOOTSEL mode (milliseconds), type=int, default=200, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS 200
#endif

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED, Optionally define a pin to use as bootloader activity LED when BOOTSEL mode is entered via reset double tap, type=int, min=0, max=47 on RP2350B, 29 otherwise, group=pico_bootsel_via_double_reset

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED_ACTIVE_LOW, Whether pin used as bootloader activity LED when BOOTSEL mode is entered via reset double tap is active low. Not supported on RP2040, type=bool, default=0, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED_ACTIVE_LOW
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED_ACTIVE_LOW 0
#endif

// PICO_CONFIG: PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK, Optionally disable either the mass storage interface (bit 0) or the PICOBOOT interface (bit 1) when entering BOOTSEL mode via double reset, type=int, min=0, max=3, default=0, group=pico_bootsel_via_double_reset
#ifndef PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK
#define PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK 0u
#endif

/** \defgroup pico_bootsel_via_double_reset pico_bootsel_via_double_reset
 *
 * \brief Optional support to make fast double reset of the system enter BOOTSEL mode
 *
 * \brief When the 'pico_bootsel_via_double_reset' library is linked, a function is
 * injected before main() which will detect when the system has been reset
 * twice in quick succession, and enter the USB ROM bootloader (BOOTSEL mode)
 * when this happens. This allows a double tap of a reset button on a
 * development board to be used to enter the ROM bootloader, provided this
 * library is always linked.
 */

#if !PICO_NO_BI_BOOTSEL_VIA_DOUBLE_RESET
bi_decl(bi_program_feature("double reset -> BOOTSEL"));
#endif

#if PICO_RP2040

// RP2040 stores a token in RAM, which is retained over assertion of the RUN pin.

static const uint32_t magic_token[] = {
        0xf01681de, 0xbd729b29, 0xd359be7a,
};

static uint32_t __uninitialized_ram(magic_location)[count_of(magic_token)];

static inline bool double_tap_flag_is_set(void) {
    for (uint i = 0; i < count_of(magic_token); i++) {
        if (magic_location[i] != magic_token[i]) {
            return false;
        }
    }
    return true;
}

static inline void set_double_tap_flag(void) {
    for (uint i = 0; i < count_of(magic_token); i++) {
        magic_location[i] = magic_token[i];
    }
}

static inline void clear_double_tap_flag(void) {
    magic_location[0] = 0;
}

#else

// Newer microcontrollers have a purpose-made register which is retained over
// RUN events, for detecting double-tap events. The ROM has built-in support
// for this, but this library can also use the same hardware feature.
// (Also, RAM is powered down when the RUN pin is asserted, so it's a bad
// place to put the token!)
//
// Note if ROM support is also enabled (via DOUBLE_TAP in OTP BOOT_FLAGS) then
// we never reach this point with the double tap flag still set. The window
// is the sum of the delay added by this library and the delay added by the
// ROM. It's not recommended to enable both, but it works.

static inline bool double_tap_flag_is_set(void) {
    return powman_hw->chip_reset & POWMAN_CHIP_RESET_DOUBLE_TAP_BITS;
}

static inline void set_double_tap_flag(void) {
    hw_set_bits(&powman_hw->chip_reset, POWMAN_CHIP_RESET_DOUBLE_TAP_BITS);
}

static inline void clear_double_tap_flag(void) {
    hw_clear_bits(&powman_hw->chip_reset, POWMAN_CHIP_RESET_DOUBLE_TAP_BITS);
}

#endif

/* Check for double reset and enter BOOTSEL mode if detected
 *
 * This function is registered to run automatically before main(). The
 * algorithm is:
 *
 *   1. Check for magic token in memory; enter BOOTSEL mode if found.
 *   2. Initialise that memory with that magic token.
 *   3. Do nothing for a short while (few hundred ms).
 *   4. Clear the magic token.
 *   5. Continue with normal boot.
 *
 * Resetting the device twice quickly will interrupt step 3, leaving the token
 * in place so that the second boot will go to the bootloader.
 */
static void __attribute__((constructor)) boot_double_tap_check(void) {
    if (!double_tap_flag_is_set()) {
        // Arm, wait, then disarm and continue booting
        set_double_tap_flag();
        busy_wait_us(PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS * 1000);
        clear_double_tap_flag();
        return;
    }

    // Detected a double reset, so enter USB bootloader
    clear_double_tap_flag();
#ifdef PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED
    const int led = PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED;
#else
    const int led = -1;
#endif
    rom_reset_usb_boot_extra(
        led,
        PICO_BOOTSEL_VIA_DOUBLE_RESET_INTERFACE_DISABLE_MASK,
        PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED_ACTIVE_LOW
    );
}

#endif
