# Used for RP2040 and RP2350

include(cmake/on_device.cmake)

# PICO_CMAKE_CONFIG: PICO_NO_FLASH, Option to default all binaries to not use flash i.e. run from SRAM, type=bool, default=0, group=build, docref=cmake-binary-type-config
option(PICO_NO_FLASH "Default binaries to not not use flash")
# PICO_CMAKE_CONFIG: PICO_COPY_TO_RAM, Option to default all binaries to copy code from flash to SRAM before running, type=bool, default=0, group=build, docref=cmake-binary-type-config
option(PICO_COPY_TO_RAM "Default binaries to copy code to RAM when booting from flash")

# COMMON
pico_add_subdirectory(common/boot_picobin_headers)
pico_add_subdirectory(common/boot_picoboot_headers)
pico_add_subdirectory(common/boot_uf2_headers)
pico_add_subdirectory(common/pico_base_headers)
pico_add_subdirectory(common/pico_usb_reset_interface_headers)

# PICO_CMAKE_CONFIG: PICO_BARE_METAL, Flag to exclude anything except base headers from the build, type=bool, default=0, group=build
if (NOT PICO_BARE_METAL)
    pico_add_subdirectory(common/pico_bit_ops_headers)
    pico_add_subdirectory(common/pico_binary_info)
    pico_add_subdirectory(common/pico_divider_headers)
    pico_add_subdirectory(common/pico_sync)
    pico_add_subdirectory(common/pico_time)
    pico_add_subdirectory(common/pico_util)
    pico_add_subdirectory(common/pico_stdlib_headers)
endif()
pico_add_subdirectory(common/hardware_claim)
#
# RP2040/RP2350 specific From standard build variants
pico_add_subdirectory(${RP2_VARIANT_DIR}/pico_platform)
pico_add_subdirectory(${RP2_VARIANT_DIR}/hardware_regs)
pico_add_subdirectory(${RP2_VARIANT_DIR}/hardware_structs)
pico_add_subdirectory(${RP2_VARIANT_DIR}/boot_stage2)

pico_add_subdirectory(rp2_common/hardware_base)
# HAL items which expose a public (inline rp2_common) functions/macro API above the raw hardware
pico_add_subdirectory(rp2_common/hardware_adc)
pico_add_subdirectory(rp2_common/hardware_boot_lock)
pico_add_subdirectory(rp2_common/hardware_clocks)
pico_add_subdirectory(rp2_common/hardware_divider)
pico_add_subdirectory(rp2_common/hardware_dma)
pico_add_subdirectory(rp2_common/hardware_exception)
pico_add_subdirectory(rp2_common/hardware_flash)
pico_add_subdirectory(rp2_common/hardware_gpio)
pico_add_subdirectory(rp2_common/hardware_i2c)
pico_add_subdirectory(rp2_common/hardware_interp)
pico_add_subdirectory(rp2_common/hardware_irq)
pico_add_subdirectory(rp2_common/hardware_pio)
pico_add_subdirectory(rp2_common/hardware_pll)
pico_add_subdirectory(rp2_common/hardware_pwm)
pico_add_subdirectory(rp2_common/hardware_resets)
if (PICO_RP2040 OR PICO_COMBINED_DOCS)
    pico_add_subdirectory(rp2_common/hardware_rtc)
endif()
pico_add_subdirectory(rp2_common/hardware_spi)
pico_add_subdirectory(rp2_common/hardware_sync)
pico_add_subdirectory(rp2_common/hardware_sync_spin_lock)
pico_add_subdirectory(rp2_common/hardware_ticks)
pico_add_subdirectory(rp2_common/hardware_timer)
pico_add_subdirectory(rp2_common/hardware_uart)
pico_add_subdirectory(rp2_common/hardware_vreg)
pico_add_subdirectory(rp2_common/hardware_watchdog)
pico_add_subdirectory(rp2_common/hardware_xip_cache)
pico_add_subdirectory(rp2_common/hardware_xosc)

if (PICO_RP2350 OR PICO_COMBINED_DOCS)
    pico_add_subdirectory(rp2_common/hardware_powman)
    # Note in spite of the name this is usable on Arm as well as RISC-V:
    pico_add_subdirectory(rp2_common/hardware_riscv_platform_timer)
    pico_add_subdirectory(rp2_common/hardware_sha256)
endif()

if (PICO_RP2350 OR PICO_COMBINED_DOCS)
    pico_add_subdirectory(rp2_common/hardware_dcp)
    pico_add_subdirectory(rp2_common/hardware_rcp)
endif()

if (PICO_RISCV OR PICO_COMBINED_DOCS)
    pico_add_subdirectory(rp2_common/hardware_riscv)
    pico_add_subdirectory(rp2_common/hardware_hazard3)
endif()

# Basic bootrom headers
pico_add_subdirectory(rp2_common/boot_bootrom_headers)
pico_add_subdirectory(rp2_common/pico_platform_compiler)
pico_add_subdirectory(rp2_common/pico_platform_sections)
pico_add_subdirectory(rp2_common/pico_platform_panic)

if (NOT PICO_BARE_METAL)
    # NOTE THE ORDERING HERE IS IMPORTANT AS SOME TARGETS CHECK ON EXISTENCE OF OTHER TARGETS
    pico_add_subdirectory(rp2_common/pico_aon_timer)
    # Helper functions to connect to data/functions in the bootrom
    pico_add_subdirectory(rp2_common/pico_bootrom)
    pico_add_subdirectory(rp2_common/pico_bootsel_via_double_reset)
    pico_add_subdirectory(rp2_common/pico_multicore)
    pico_add_subdirectory(rp2_common/pico_unique_id)

    pico_add_subdirectory(rp2_common/pico_atomic)
    pico_add_subdirectory(rp2_common/pico_bit_ops)
    pico_add_subdirectory(rp2_common/pico_divider)
    pico_add_subdirectory(rp2_common/pico_double)
    pico_add_subdirectory(rp2_common/pico_int64_ops)
    pico_add_subdirectory(rp2_common/pico_flash)
    pico_add_subdirectory(rp2_common/pico_float)
    pico_add_subdirectory(rp2_common/pico_mem_ops)
    pico_add_subdirectory(rp2_common/pico_malloc)
    pico_add_subdirectory(rp2_common/pico_printf)
    pico_add_subdirectory(rp2_common/pico_rand)

    if (PICO_RP2350 OR PICO_COMBINED_DOCS)
        pico_add_subdirectory(rp2_common/pico_sha256)
    endif()

    pico_add_subdirectory(rp2_common/pico_stdio_semihosting)
    pico_add_subdirectory(rp2_common/pico_stdio_uart)
    pico_add_subdirectory(rp2_common/pico_stdio_rtt)

    if (NOT PICO_RISCV)
         pico_add_subdirectory(rp2_common/cmsis)
    endif()
    pico_add_subdirectory(rp2_common/tinyusb)
    pico_add_subdirectory(rp2_common/pico_stdio_usb)
    pico_add_subdirectory(rp2_common/pico_i2c_slave)

    # networking libraries - note dependency order is important
    pico_add_subdirectory(rp2_common/pico_async_context)
    pico_add_subdirectory(rp2_common/pico_btstack)
    pico_add_subdirectory(rp2_common/pico_cyw43_driver)
    pico_add_subdirectory(rp2_common/pico_lwip)
    pico_add_subdirectory(rp2_common/pico_cyw43_arch)
    pico_add_subdirectory(rp2_common/pico_mbedtls)

    pico_add_subdirectory(rp2_common/pico_time_adapter)

    pico_add_subdirectory(rp2_common/pico_crt0)
    pico_add_subdirectory(rp2_common/pico_clib_interface)
    pico_add_subdirectory(rp2_common/pico_cxx_options)
    pico_add_subdirectory(rp2_common/pico_standard_binary_info)
    pico_add_subdirectory(rp2_common/pico_standard_link)

    pico_add_subdirectory(rp2_common/pico_fix)

    # at the end as it includes a lot of other stuff
    pico_add_subdirectory(rp2_common/pico_runtime_init)
    pico_add_subdirectory(rp2_common/pico_runtime)

    # this requires all the pico_stdio_ libraries
    pico_add_subdirectory(rp2_common/pico_stdio)
    # this requires runtime
    pico_add_subdirectory(rp2_common/pico_stdlib)
endif()

# configure doxygen directories
#pico_add_doxygen(${COMMON_DIR})
#pico_add_doxygen(${RP2_VARIANT_DIR})
pico_add_doxygen_exclude(${RP2_VARIANT_DIR}/hardware_regs) # very very big
# but we DO want dreq.h; it doesn't change much, so lets just use configure_file
configure_file(${RP2_VARIANT_DIR}/hardware_regs/include/hardware/regs/dreq.h ${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen/dreq.h COPYONLY)
# also intctrl.h
configure_file(${RP2_VARIANT_DIR}/hardware_regs/include/hardware/regs/intctrl.h ${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen/intctrl.h COPYONLY)
pico_add_doxygen(${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen)

#pico_add_doxygen(rp2_common)
pico_add_doxygen_exclude(rp2_common/cmsis) # very big
