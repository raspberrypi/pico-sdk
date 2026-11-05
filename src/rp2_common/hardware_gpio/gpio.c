/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "hardware/structs/io_bank0.h"
#include "hardware/irq.h"

#if LIB_PICO_BINARY_INFO
#include "pico/binary_info.h"
#endif

static gpio_irq_callback_t callbacks[NUM_CORES];
// a 1 bit means the IRQ is handled by a raw IRQ handler
#if NUM_BANK0_GPIOS > 32
typedef uint64_t raw_irq_mask_type_t;
#else
typedef uint32_t raw_irq_mask_type_t;
#endif

static raw_irq_mask_type_t raw_irq_mask[NUM_CORES];
#if PICO_SUPPORT_QSPI_GPIOS
// a 1 bit (indexed from QSPI_GPIO_MIN) means the IRQ is handled by a raw IRQ handler
static uint8_t raw_qspi_irq_mask[NUM_CORES];
static_assert(QSPI_GPIO_MAX - QSPI_GPIO_MIN < 8, "");

// Check that layout of bank0 and qspi is the same
#define static_assert_same_field(reg, bank0_reg, field) static_assert(reg##_##field##_BITS == bank0_reg##_##field##_BITS, "")
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, IE);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, OD);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, PUE);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, PDE);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, SCHMITT);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, SLEWFAST);
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, DRIVE);
#if HAS_PADS_BANK0_ISOLATION
static_assert_same_field(PADS_QSPI_GPIO_QSPI_SCLK, PADS_BANK0_GPIO0, ISO);
#endif
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_STATUS, IO_BANK0_GPIO0_STATUS, INFROMPAD);
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_CTRL, IO_BANK0_GPIO0_CTRL, FUNCSEL);
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_CTRL, IO_BANK0_GPIO0_CTRL, OUTOVER);
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_CTRL, IO_BANK0_GPIO0_CTRL, OEOVER);
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_CTRL, IO_BANK0_GPIO0_CTRL, INOVER);
static_assert_same_field(IO_QSPI_GPIO_QSPI_SCLK_CTRL, IO_BANK0_GPIO0_CTRL, IRQOVER);
#if !PICO_RP2040
static_assert_same_field(IO_QSPI_USBPHY_DP_STATUS, IO_BANK0_GPIO0_STATUS, INFROMPAD);
static_assert_same_field(IO_QSPI_USBPHY_DP_CTRL, IO_BANK0_GPIO0_CTRL, FUNCSEL);
static_assert_same_field(IO_QSPI_USBPHY_DP_CTRL, IO_BANK0_GPIO0_CTRL, OUTOVER);
static_assert_same_field(IO_QSPI_USBPHY_DP_CTRL, IO_BANK0_GPIO0_CTRL, OEOVER);
static_assert_same_field(IO_QSPI_USBPHY_DP_CTRL, IO_BANK0_GPIO0_CTRL, INOVER);
static_assert_same_field(IO_QSPI_USBPHY_DP_CTRL, IO_BANK0_GPIO0_CTRL, IRQOVER);
static_assert_same_field(IO_QSPI_USBPHY_DM_STATUS, IO_BANK0_GPIO0_STATUS, INFROMPAD);
static_assert_same_field(IO_QSPI_USBPHY_DM_CTRL, IO_BANK0_GPIO0_CTRL, FUNCSEL);
static_assert_same_field(IO_QSPI_USBPHY_DM_CTRL, IO_BANK0_GPIO0_CTRL, OUTOVER);
static_assert_same_field(IO_QSPI_USBPHY_DM_CTRL, IO_BANK0_GPIO0_CTRL, OEOVER);
static_assert_same_field(IO_QSPI_USBPHY_DM_CTRL, IO_BANK0_GPIO0_CTRL, INOVER);
static_assert_same_field(IO_QSPI_USBPHY_DM_CTRL, IO_BANK0_GPIO0_CTRL, IRQOVER);
#endif
#undef static_assert_same_field

static io_rw_32 *gpio_lookup_qspi_pad_reg(uint gpio) {
    check_qspi_gpio_param(gpio);
    return &pads_qspi_hw->io[get_qspi_gpio_pad_index(gpio)];
}

static void gpio_pads_set_bits(uint gpio, uint32_t bits) {
    if (check_is_qspi_gpio(gpio)) {
        hw_set_bits(gpio_lookup_qspi_pad_reg(gpio), bits);
        return;
    }
    check_gpio_param(gpio);
    pads_bank0_set_bits(gpio, bits);
}

static void gpio_pads_clear_bits(uint gpio, uint32_t bits) {
    if (check_is_qspi_gpio(gpio)) {
        hw_clear_bits(gpio_lookup_qspi_pad_reg(gpio), bits);
        return;
    }
    check_gpio_param(gpio);
    pads_bank0_clear_bits(gpio, bits);
}

static void gpio_pads_write_masked(uint gpio, uint32_t values, uint32_t write_mask) {
    if (check_is_qspi_gpio(gpio)) {
        hw_write_masked(gpio_lookup_qspi_pad_reg(gpio), values, write_mask);
        return;
    }
    check_gpio_param(gpio);
    pads_bank0_write_masked(gpio, values, write_mask);
}

static uint32_t gpio_pads_read(uint gpio) {
    if (check_is_qspi_gpio(gpio)) {
        return *gpio_lookup_qspi_pad_reg(gpio);
    }
    check_gpio_param(gpio);
    return pads_bank0_read(gpio);
}

static io_bank0_status_ctrl_hw_t *gpio_lookup_io_reg(uint gpio) {
    if (check_is_qspi_gpio(gpio)) {
        check_qspi_gpio_param(gpio);
        return (io_bank0_status_ctrl_hw_t *)io_qspi_hw + get_qspi_gpio_io_offset(gpio);
    }
    check_gpio_param(gpio);
    return &io_bank0_hw->io[gpio];
}

static_assert(sizeof(io_qspi_status_ctrl_hw_t) == sizeof(io_bank0_status_ctrl_hw_t), "");
static_assert(IO_QSPI_GPIO_QSPI_SCLK_CTRL_OFFSET == (QSPI_GPIO_SCLK - QSPI_GPIO_MIN) * sizeof(io_qspi_status_ctrl_hw_t) + 4, "");
static_assert(IO_QSPI_GPIO_QSPI_SD3_CTRL_OFFSET == (QSPI_GPIO_SD3 - QSPI_GPIO_MIN) * sizeof(io_qspi_status_ctrl_hw_t) + 4, "");

#define check_gpio_any_param(gpio) ((void)0) // checked by the accessors/lookups
#define gpio_io_reg(gpio) (*gpio_lookup_io_reg(gpio))
// USB DP/DM have no pads in PADS_QSPI (their pad controls are in the USB PHY)
#define gpio_has_pad(gpio) (!check_is_qspi_gpio(gpio) || check_qspi_gpio_has_pad(gpio))
#else
#define check_gpio_any_param(gpio) check_gpio_param(gpio)
#define gpio_pads_set_bits(gpio, bits) pads_bank0_set_bits(gpio, bits)
#define gpio_pads_clear_bits(gpio, bits) pads_bank0_clear_bits(gpio, bits)
#define gpio_pads_write_masked(gpio, values, write_mask) pads_bank0_write_masked(gpio, values, write_mask)
#define gpio_pads_read(gpio) pads_bank0_read(gpio)
#define gpio_io_reg(gpio) (io_bank0_hw->io[gpio])
#define gpio_has_pad(gpio) true
#endif

// Get the raw value from the pin, bypassing any muxing or overrides.
int gpio_get_pad(uint gpio) {
    check_gpio_any_param(gpio);
    if (gpio_has_pad(gpio)) {
        gpio_pads_set_bits(gpio, PADS_BANK0_GPIO0_IE_BITS);
    }
    return (gpio_io_reg(gpio).status & IO_BANK0_GPIO0_STATUS_INFROMPAD_BITS)
            >> IO_BANK0_GPIO0_STATUS_INFROMPAD_LSB;
}

/// \tag::gpio_set_function[]
// Select function for this GPIO, and ensure input/output are enabled at the pad.
// This also clears the input/output/irq override bits.
void gpio_set_function(uint gpio, gpio_function_t fn) {
    check_gpio_any_param(gpio);
    invalid_params_if(HARDWARE_GPIO, ((uint32_t)fn << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB) & ~IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS);
    // Set input enable on, output disable off
    if (gpio_has_pad(gpio)) {
        gpio_pads_write_masked(gpio,
                               PADS_BANK0_GPIO0_IE_BITS,
                               PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );
    }
    // Zero all fields apart from fsel; we want this IO to do what the peripheral tells it.
    // This doesn't affect e.g. pullup/pulldown, as these are in pad controls.
    gpio_io_reg(gpio).ctrl = fn << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
#if HAS_PADS_BANK0_ISOLATION
    // Remove pad isolation now that the correct peripheral is in control of the pad
    if (gpio_has_pad(gpio)) {
        gpio_pads_clear_bits(gpio, PADS_BANK0_GPIO0_ISO_BITS);
    }
#endif
}
/// \end::gpio_set_function[]

gpio_function_t gpio_get_function(uint gpio) {
    check_gpio_any_param(gpio);
    return (gpio_function_t) ((gpio_io_reg(gpio).ctrl & IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) >> IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB);
}

// Note that, on RP2040, setting both pulls enables a "bus keep" function,
// i.e. weak pull to whatever is current high/low state of GPIO.
void gpio_set_pulls(uint gpio, bool up, bool down) {
    check_gpio_any_param(gpio);
    gpio_pads_write_masked(
            gpio,
            (bool_to_bit(up) << PADS_BANK0_GPIO0_PUE_LSB) | (bool_to_bit(down) << PADS_BANK0_GPIO0_PDE_LSB),
            PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS
    );
}

// Direct override for per-GPIO IRQ signal
void gpio_set_irqover(uint gpio, uint value) {
    check_gpio_any_param(gpio);
    hw_write_masked(&gpio_io_reg(gpio).ctrl,
                   value << IO_BANK0_GPIO0_CTRL_IRQOVER_LSB,
                   IO_BANK0_GPIO0_CTRL_IRQOVER_BITS
    );
}

// Direct overrides for pad controls
void gpio_set_inover(uint gpio, uint value) {
    check_gpio_any_param(gpio);
    hw_write_masked(&gpio_io_reg(gpio).ctrl,
                   value << IO_BANK0_GPIO0_CTRL_INOVER_LSB,
                   IO_BANK0_GPIO0_CTRL_INOVER_BITS
    );
}

void gpio_set_outover(uint gpio, uint value) {
    check_gpio_any_param(gpio);
    hw_write_masked(&gpio_io_reg(gpio).ctrl,
                   value << IO_BANK0_GPIO0_CTRL_OUTOVER_LSB,
                   IO_BANK0_GPIO0_CTRL_OUTOVER_BITS
    );
}

void gpio_set_oeover(uint gpio, uint value) {
    check_gpio_any_param(gpio);
    hw_write_masked(&gpio_io_reg(gpio).ctrl,
                   value << IO_BANK0_GPIO0_CTRL_OEOVER_LSB,
                   IO_BANK0_GPIO0_CTRL_OEOVER_BITS
    );
}

void gpio_set_input_hysteresis_enabled(uint gpio, bool enabled) {
    check_gpio_any_param(gpio);
    if (enabled)
        gpio_pads_set_bits(gpio, PADS_BANK0_GPIO0_SCHMITT_BITS);
    else
        gpio_pads_clear_bits(gpio, PADS_BANK0_GPIO0_SCHMITT_BITS);
}


bool gpio_is_input_hysteresis_enabled(uint gpio) {
    check_gpio_any_param(gpio);
    return (gpio_pads_read(gpio) & PADS_BANK0_GPIO0_SCHMITT_BITS) != 0;
}

void gpio_set_slew_rate(uint gpio, enum gpio_slew_rate slew) {
    check_gpio_any_param(gpio);
    gpio_pads_write_masked(gpio,
                    (uint)slew << PADS_BANK0_GPIO0_SLEWFAST_LSB,
                    PADS_BANK0_GPIO0_SLEWFAST_BITS
    );
}

enum gpio_slew_rate gpio_get_slew_rate(uint gpio) {
    check_gpio_any_param(gpio);
    return (enum gpio_slew_rate)((gpio_pads_read(gpio)
                                  & PADS_BANK0_GPIO0_SLEWFAST_BITS)
            >> PADS_BANK0_GPIO0_SLEWFAST_LSB);
}


// Enum encoding should match hardware encoding on RP2040
static_assert(PADS_BANK0_GPIO0_DRIVE_VALUE_8MA == GPIO_DRIVE_STRENGTH_8MA, "");
void gpio_set_drive_strength(uint gpio, enum gpio_drive_strength drive) {
    check_gpio_any_param(gpio);
    gpio_pads_write_masked(gpio,
                    (uint)drive << PADS_BANK0_GPIO0_DRIVE_LSB,
                    PADS_BANK0_GPIO0_DRIVE_BITS
    );
}

enum gpio_drive_strength gpio_get_drive_strength(uint gpio) {
    check_gpio_any_param(gpio);
    return (enum gpio_drive_strength)((gpio_pads_read(gpio)
                                       & PADS_BANK0_GPIO0_DRIVE_BITS)
            >> PADS_BANK0_GPIO0_DRIVE_LSB);
}

static void gpio_default_irq_handler(void) {
    uint core = get_core_num();
    gpio_irq_callback_t callback = callbacks[core];
    io_bank0_irq_ctrl_hw_t *irq_ctrl_base = get_core_irq_ctrl(core);
    for (uint gpio = 0; gpio < NUM_BANK0_GPIOS; gpio+=8) {
        uint32_t events8 = irq_ctrl_base->ints[gpio >> 3u];
        // note we assume events8 is 0 for non-existent GPIO
        for(uint i=gpio;events8 && i<gpio+8;i++) {
            uint32_t events = events8 & 0xfu;
            if (events && !(raw_irq_mask[core] & (1ull << i))) {
                io_bank0_hw->intr[i / 8] = events << (4 * (i % 8));
                if (callback) {
                    callback(i, events);
                }
            }
            events8 >>= 4;
        }
    }
#if PICO_SUPPORT_QSPI_GPIOS
    // this handler is registered on both IO_IRQ_BANK0 and IO_IRQ_QSPI
    uint32_t events8 = get_qspi_core_irq_ctrl(core)->ints;
    for (uint i = 0; events8; i++) {
        uint32_t events = events8 & 0xfu;
        if (events && !(raw_qspi_irq_mask[core] & (1u << i))) {
            io_qspi_hw->intr = events << (4 * i);
            if (callback) {
                callback(QSPI_GPIO_MIN + i, events);
            }
        }
        events8 >>= 4;
    }
#endif
}

static void _gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled, io_bank0_irq_ctrl_hw_t *irq_ctrl_base) {
    // Clear stale events which might cause immediate spurious handler entry
    gpio_acknowledge_irq(gpio, events);

    io_rw_32 *en_reg = &irq_ctrl_base->inte[gpio / 8];
    events <<= 4 * (gpio % 8);

    if (enabled)
        hw_set_bits(en_reg, events);
    else
        hw_clear_bits(en_reg, events);
}

#if PICO_SUPPORT_QSPI_GPIOS
static void _gpio_set_qspi_irq_enabled(uint gpio, uint32_t events, bool enabled, io_qspi_irq_ctrl_hw_t *irq_ctrl_base) {
    // Clear stale events which might cause immediate spurious handler entry
    gpio_acknowledge_irq(gpio, events);

    events <<= 4 * get_qspi_gpio_io_offset(gpio);
    if (enabled)
        hw_set_bits(&irq_ctrl_base->inte, events);
    else
        hw_clear_bits(&irq_ctrl_base->inte, events);
}
#endif

void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled) {
#if PICO_SUPPORT_QSPI_GPIOS
    if (check_is_qspi_gpio(gpio)) {
        check_qspi_gpio_param(gpio);
        assert(!enabled || irq_has_handler(IO_IRQ_QSPI));
        _gpio_set_qspi_irq_enabled(gpio, events, enabled, get_qspi_core_irq_ctrl(get_core_num()));
        return;
    }
#endif
    // either this call disables the interrupt or callback should already be set.
    // this protects against enabling the interrupt without callback set
    assert(!enabled || irq_has_handler(IO_IRQ_BANK0));

    // Separate mask/force/status per-core, so check which core called, and
    // set the relevant IRQ controls.
    io_bank0_irq_ctrl_hw_t *irq_ctrl_base = get_core_irq_ctrl(get_core_num());
    _gpio_set_irq_enabled(gpio, events, enabled, irq_ctrl_base);
}

void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events, bool enabled, gpio_irq_callback_t callback) {
    // when enabling, first set callback, then enable the interrupt
    // when disabling, first disable the interrupt, then clear callback
    if (enabled) gpio_set_irq_callback(callback);
    gpio_set_irq_enabled(gpio, events, enabled);
    if (!enabled) gpio_set_irq_callback(callback);
#if PICO_SUPPORT_QSPI_GPIOS
    if (enabled && check_is_qspi_gpio(gpio)) {
        irq_set_enabled(IO_IRQ_QSPI, true);
        return;
    }
#endif
    if (enabled) irq_set_enabled(IO_IRQ_BANK0, true);
}

void gpio_set_irq_callback(gpio_irq_callback_t callback) {
    uint core = get_core_num();
    if (callbacks[core]) {
        if (!callback) {
            irq_remove_handler(IO_IRQ_BANK0, gpio_default_irq_handler);
#if PICO_SUPPORT_QSPI_GPIOS
            irq_remove_handler(IO_IRQ_QSPI, gpio_default_irq_handler);
#endif
        }
        callbacks[core] = callback;
    } else if (callback) {
        callbacks[core] = callback;
        irq_add_shared_handler(IO_IRQ_BANK0, gpio_default_irq_handler, GPIO_IRQ_CALLBACK_ORDER_PRIORITY);
#if PICO_SUPPORT_QSPI_GPIOS
        irq_add_shared_handler(IO_IRQ_QSPI, gpio_default_irq_handler, GPIO_IRQ_CALLBACK_ORDER_PRIORITY);
#endif
    }
}

void gpio_add_raw_irq_handler_with_order_priority_masked(uint32_t gpio_mask, irq_handler_t handler, uint8_t order_priority) {
    hard_assert(!(raw_irq_mask[get_core_num()] & gpio_mask)); // should not add multiple handlers for the same event
    raw_irq_mask[get_core_num()] |= gpio_mask;
    irq_add_shared_handler(IO_IRQ_BANK0, handler, order_priority);
}

void gpio_add_raw_irq_handler_with_order_priority_masked64(uint64_t gpio_mask, irq_handler_t handler, uint8_t order_priority) {
#if PICO_SUPPORT_QSPI_GPIOS
    uint qspi_mask = (uint)(gpio_mask >> QSPI_GPIO_MIN);
    if (qspi_mask) {
        hard_assert(!(raw_qspi_irq_mask[get_core_num()] & qspi_mask)); // should not add multiple handlers for the same event
        raw_qspi_irq_mask[get_core_num()] |= (uint8_t)qspi_mask;
        irq_add_shared_handler(IO_IRQ_QSPI, handler, order_priority);
        gpio_mask &= (1ull << QSPI_GPIO_MIN) - 1;
        if (!gpio_mask) return;
    }
#endif
    hard_assert(!(raw_irq_mask[get_core_num()] & gpio_mask)); // should not add multiple handlers for the same event
    raw_irq_mask[get_core_num()] |= (raw_irq_mask_type_t) gpio_mask;
    irq_add_shared_handler(IO_IRQ_BANK0, handler, order_priority);
}

void gpio_add_raw_irq_handler_masked(uint32_t gpio_mask, irq_handler_t handler) {
    gpio_add_raw_irq_handler_with_order_priority_masked(gpio_mask, handler, GPIO_RAW_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
}

void gpio_add_raw_irq_handler_masked64(uint64_t gpio_mask, irq_handler_t handler) {
    gpio_add_raw_irq_handler_with_order_priority_masked64(gpio_mask, handler, GPIO_RAW_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
}

void gpio_remove_raw_irq_handler_masked(uint32_t gpio_mask, irq_handler_t handler) {
    assert(raw_irq_mask[get_core_num()] & gpio_mask); // should not remove handlers that are not added
    irq_remove_handler(IO_IRQ_BANK0, handler);
    raw_irq_mask[get_core_num()] &= ~gpio_mask;
}

void gpio_remove_raw_irq_handler_masked64(uint64_t gpio_mask, irq_handler_t handler) {
#if PICO_SUPPORT_QSPI_GPIOS
    uint qspi_mask = (uint)(gpio_mask >> QSPI_GPIO_MIN);
    if (qspi_mask) {
        assert(raw_qspi_irq_mask[get_core_num()] & qspi_mask); // should not remove handlers that are not added
        irq_remove_handler(IO_IRQ_QSPI, handler);
        raw_qspi_irq_mask[get_core_num()] &= (uint8_t)~qspi_mask;
        gpio_mask &= (1ull << QSPI_GPIO_MIN) - 1;
        if (!gpio_mask) return;
    }
#endif
    assert(raw_irq_mask[get_core_num()] & gpio_mask); // should not remove handlers that are not added
    irq_remove_handler(IO_IRQ_BANK0, handler);
    raw_irq_mask[get_core_num()] &= (raw_irq_mask_type_t)~gpio_mask;
}

void gpio_set_dormant_irq_enabled(uint gpio, uint32_t events, bool enabled) {
#if PICO_SUPPORT_QSPI_GPIOS
    if (check_is_qspi_gpio(gpio)) {
        check_qspi_gpio_param(gpio);
        _gpio_set_qspi_irq_enabled(gpio, events, enabled, &io_qspi_hw->dormant_wake_irq_ctrl);
        return;
    }
#endif
    check_gpio_param(gpio);
    io_bank0_irq_ctrl_hw_t *irq_ctrl_base = &io_bank0_hw->dormant_wake_irq_ctrl;
    _gpio_set_irq_enabled(gpio, events, enabled, irq_ctrl_base);
}

#define DEBUG_PIN_MASK (((1u << PICO_DEBUG_PIN_COUNT)-1) << PICO_DEBUG_PIN_BASE)
void gpio_debug_pins_init(void) {
    gpio_init_mask(DEBUG_PIN_MASK);
    gpio_set_dir_masked(DEBUG_PIN_MASK, DEBUG_PIN_MASK);
#if LIB_PICO_BINARY_INFO
    bi_decl_if_func_used(bi_pin_mask_with_names(DEBUG_PIN_MASK, "Debug"));
#endif
}

void gpio_set_input_enabled(uint gpio, bool enabled) {
    check_gpio_any_param(gpio);
    if (enabled)
        gpio_pads_set_bits(gpio, PADS_BANK0_GPIO0_IE_BITS);
    else
        gpio_pads_clear_bits(gpio, PADS_BANK0_GPIO0_IE_BITS);
}

void gpio_init(uint gpio) {
    gpio_set_dir(gpio, GPIO_IN);
    gpio_put(gpio, 0);
    gpio_set_function(gpio, GPIO_FUNC_SIO);
}

void gpio_deinit(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_NULL);
}

void gpio_set_function_masked(uint32_t gpio_mask, gpio_function_t fn) {
    for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
        if (gpio_mask & 1u) {
            gpio_set_function(i, fn);
        }
        gpio_mask >>= 1;
        if (!gpio_mask) break;
    }
}

void gpio_init_mask(uint32_t gpio_mask) {
    gpio_set_dir_in_masked(gpio_mask);
    gpio_clr_mask(gpio_mask);
    gpio_set_function_masked(gpio_mask, GPIO_FUNC_SIO);
}

// these functions are provided (in gpio.h) as inlined calls to the 32 bit versions if we have <= 32 GPIOs
#if PICO_SUPPORT_GT32_GPIOS
#if PICO_SUPPORT_QSPI_GPIOS
#define NUM_MASK64_GPIOS (QSPI_GPIO_MAX + 1)
#else
#define NUM_MASK64_GPIOS NUM_BANK0_GPIOS
#endif
void gpio_set_function_masked64(uint64_t gpio_mask, gpio_function_t fn) {
    for (uint i = 0; i < NUM_MASK64_GPIOS; i++) {
        if (gpio_mask & 1u) {
            gpio_set_function(i, fn);
        }
        gpio_mask >>= 1;
        if (!gpio_mask) break;
    }
}

void gpio_init_mask64(uint64_t gpio_mask) {
    gpio_set_dir_in_masked64(gpio_mask);
    gpio_clr_mask64(gpio_mask);
    gpio_set_function_masked64(gpio_mask, GPIO_FUNC_SIO);
}

#endif