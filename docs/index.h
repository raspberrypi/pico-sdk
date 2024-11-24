/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Here to organize documentation order

// for some reason cond/endcond work better here than if/endif

/**
 * \defgroup hardware Hardware APIs
 * This group of libraries provides a thin and efficient C API / abstractions to access the RP-series microcontroller hardware without having to read and write
 * hardware registers directly.
 * @{
 * \cond hardware_adc \defgroup hardware_adc hardware_adc \endcond
 * \cond hardware_base \defgroup hardware_base hardware_base \endcond
 * \cond hardware_bootlock \defgroup hardware_bootlock hardware_bootlock \endcond
 * \cond hardware_claim \defgroup hardware_claim hardware_claim \endcond
 * \cond hardware_clocks \defgroup hardware_clocks hardware_clocks \endcond
 * \cond hardware_divider \defgroup hardware_divider hardware_divider \endcond
 * \cond hardware_dcp \defgroup hardware_dcp hardware_dcp \endcond
 * \cond hardware_dma \defgroup hardware_dma hardware_dma \endcond
 * \cond hardware_exception \defgroup hardware_exception hardware_exception \endcond
 * \cond hardware_flash \defgroup hardware_flash hardware_flash \endcond
 * \cond hardware_gpio \defgroup hardware_gpio hardware_gpio \endcond
 * \cond hardware_hazard3 \defgroup hardware_hazard3 hardware_hazard3 \endcond
 * \cond hardware_i2c \defgroup hardware_i2c hardware_i2c \endcond
 * \cond hardware_interp \defgroup hardware_interp hardware_interp \endcond
 * \cond hardware_irq \defgroup hardware_irq hardware_irq \endcond
 * \cond hardware_pio \defgroup hardware_pio hardware_pio \endcond
 * \cond hardware_pll \defgroup hardware_pll hardware_pll \endcond
 * \cond hardware_powman \defgroup hardware_powman hardware_powman \endcond
 * \cond hardware_pwm \defgroup hardware_pwm hardware_pwm \endcond
 * \cond hardware_pwm \defgroup hardware_pwm hardware_pwm \endcond
 * \cond hardware_resets \defgroup hardware_resets hardware_resets \endcond
 * \cond hardware_riscv \defgroup hardware_riscv hardware_riscv \endcond
 * \cond hardware_riscv_platform_timer \defgroup hardware_riscv_platform_timer hardware_riscv_platform_timer \endcond
 * \cond hardware_rtc \defgroup hardware_rtc hardware_rtc \endcond
 * \cond hardware_rcp \defgroup hardware_rcp hardware_rcp \endcond
 * \cond hardware_spi \defgroup hardware_spi hardware_spi \endcond
 * \cond hardware_sha256 \defgroup hardware_sha256 hardware_sha256 \endcond
 * \cond hardware_sync \defgroup hardware_sync hardware_sync \endcond
 * \cond hardware_ticks \defgroup hardware_ticks hardware_ticks \endcond
 * \cond hardware_timer \defgroup hardware_timer hardware_timer \endcond
 * \cond hardware_uart \defgroup hardware_uart hardware_uart \endcond
 * \cond hardware_vreg \defgroup hardware_vreg hardware_vreg \endcond
 * \cond hardware_watchdog \defgroup hardware_watchdog hardware_watchdog \endcond
 * \cond hardware_xip_cache \defgroup hardware_xip_cache hardware_xip_cache \endcond
 * \cond hardware_xosc \defgroup hardware_xosc hardware_xosc \endcond
 * \cond hardware_powman hardware_powman
 * \cond hardware_hazard3 hardware_hazard3
 * \cond hardware_riscv hardware_riscv

 * @}
 *
 * \defgroup high_level High Level APIs
 * This group of libraries provide higher level functionality that isn't hardware related or provides a richer
 * set of functionality above the basic hardware interfaces
 * @{
 * \cond pico_aon_timer \defgroup pico_aon_timer pico_aon_timer \endcond
 * \cond pico_async_context \defgroup pico_async_context pico_async_context \endcond
 * \cond pico_bootsel_via_double_reset \defgroup pico_bootsel_via_double_reset pico_bootsel_via_double_reset \endcond
 * \cond pico_flash \defgroup pico_flash pico_flash \endcond
 * \cond pico_i2c_slave \defgroup pico_i2c_slave pico_i2c_slave \endcond
 * \cond pico_multicore \defgroup pico_multicore pico_multicore \endcond
 * \cond pico_rand \defgroup pico_rand pico_rand \endcond
 * \cond pico_sha256 \defgroup pico_sha256 pico_sha256 \endcond
 * \cond pico_stdlib \defgroup pico_stdlib pico_stdlib \endcond
 * \cond pico_sync \defgroup pico_sync pico_sync \endcond
 * \cond pico_time \defgroup pico_time pico_time \endcond
 * \cond pico_unique_id \defgroup pico_unique_id pico_unique_id \endcond
 * \cond pico_util \defgroup pico_util pico_util \endcond
 * @}
 *
 * \defgroup third_party Third-party Libraries
 * Third party libraries for implementing high level functionality.
 * @{
 * \cond tinyusb
 * \defgroup tinyusb_device tinyusb_device
 * \defgroup tinyusb_host tinyusb_host
 * \endcond
 * @}
 *
 * \defgroup networking Networking Libraries
 * Functions for implementing networking
 * @{
 * \cond pico_btstack \defgroup pico_btstack pico_btstack \endcond
 * \cond pico_lwip \defgroup pico_lwip pico_lwip \endcond
 * \cond pico_cyw43_driver \defgroup pico_cyw43_driver pico_cyw43_driver \endcond
 * \cond pico_cyw43_arch \defgroup pico_cyw43_arch pico_cyw43_arch \endcond
 * @}
 *
 * \defgroup runtime Runtime Infrastructure
 * Libraries that are used to provide efficient implementation of certain
 * language level and C library functions, as well as CMake INTERFACE libraries
 * abstracting the compilation and link steps in the SDK
 * @{
 * \cond boot_stage2 \defgroup boot_stage2 boot_stage2 \endcond
 * \cond pico_atomic \defgroup pico_atomic pico_atomic \endcond
 * \cond pico_base_headers \defgroup pico_base pico_base \endcond
 * \cond pico_binary_info \defgroup pico_binary_info pico_binary_info \endcond
 * \cond pico_bootrom \defgroup pico_bootrom pico_bootrom \endcond
 * \cond pico_bit_ops \defgroup pico_bit_ops pico_bit_ops \endcond
 * \cond pico_cxx_options \defgroup pico_cxx_options pico_cxx_options \endcond
 * \cond pico_clib_interface \defgroup pico_clib_interface pico_clib_interface \endcond
 * \cond pico_crt0 \defgroup pico_crt0 pico_crt0 \endcond
 * \cond pico_divider \defgroup pico_divider pico_divider \endcond
 * \cond pico_double \defgroup pico_double pico_double \endcond
 * \cond pico_float \defgroup pico_float pico_float \endcond
 * \cond pico_int64_ops \defgroup pico_int64_ops pico_int64_ops \endcond
 * \cond pico_malloc \defgroup pico_malloc pico_malloc \endcond
 * \cond pico_mem_ops \defgroup pico_mem_ops pico_mem_ops \endcond
 * \cond pico_platform \defgroup pico_platform pico_platform \endcond
 * \cond pico_printf \defgroup pico_printf pico_printf \endcond
 * \cond pico_runtime \defgroup pico_runtime pico_runtime \endcond
 * \cond pico_runtime_init \defgroup pico_runtime_init pico_runtime_init \endcond
 * \cond pico_stdio \defgroup pico_stdio pico_stdio \endcond
 * \cond pico_standard_binary_info \defgroup pico_standard_binary_info pico_standard_binary_info \endcond
 * \cond pico_standard_link \defgroup pico_standard_link pico_standard_link \endcond
 * @}
 *
 * \defgroup misc External API Headers
 * Headers for interfaces that are shared with code outside of the SDK
 * @{
 * \cond boot_picobin_headers \defgroup boot_picobin_headers boot_picobin_headers \endcond
 * \cond boot_picoboot_headers \defgroup boot_picoboot_headers boot_picoboot_headers \endcond
 * \cond boot_uf2_headers \defgroup boot_uf2_headers boot_uf2_headers \endcond
 * \cond pico_usb_reset_interface_headers \defgroup pico_usb_reset_interface_headers pico_usb_reset_interface_headers \endcond
 * @}
*/
