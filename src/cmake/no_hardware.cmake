macro(pico_set_float_implementation TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_double_implementation TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_binary_type TARGET IMPL)
    # ignore
endmacro()

macro(pico_set_boot_stage2 TARGET IMPL)
    # ignore
endmacro()

set(PICO_HOST_DIR "${CMAKE_CURRENT_LIST_DIR}/host" CACHE INTERNAL "")
function(pico_define_boot_stage2 NAME)
    add_executable(${NAME} ${PICO_HOST_DIR}/boot_stage2.c)
endfunction()

function(pico_add_extra_outputs TARGET)
endfunction()

set(PICO_NO_HARDWARE "1" CACHE INTERNAL "")
set(PICO_ON_DEVICE "0" CACHE INTERNAL "")

# RP2040/RP2350 specific From standard build variants
pico_add_subdirectory(${RP2_VARIANT_DIR}/hardware_regs)
pico_add_subdirectory(${RP2_VARIANT_DIR}/hardware_structs)

# common
pico_add_subdirectory(${COMMON_DIR}/boot_picobin_headers)
pico_add_subdirectory(${COMMON_DIR}/boot_picoboot_headers)
pico_add_subdirectory(${COMMON_DIR}/boot_uf2_headers)
pico_add_subdirectory(${COMMON_DIR}/hardware_claim)
pico_add_subdirectory(${COMMON_DIR}/pico_base_headers)
pico_add_subdirectory(${COMMON_DIR}/pico_usb_reset_interface_headers)
pico_add_subdirectory(${COMMON_DIR}/pico_bit_ops_headers)
pico_add_subdirectory(${COMMON_DIR}/pico_binary_info)
pico_add_subdirectory(${COMMON_DIR}/pico_divider_headers)
pico_add_subdirectory(${COMMON_DIR}/pico_sync)
pico_add_subdirectory(${COMMON_DIR}/pico_time)
pico_add_subdirectory(${COMMON_DIR}/pico_util)
pico_add_subdirectory(${COMMON_DIR}/pico_stdlib_headers)

# host-specific
pico_add_subdirectory(${HOST_DIR}/hardware_divider)
pico_add_subdirectory(${HOST_DIR}/hardware_gpio)
pico_add_subdirectory(${HOST_DIR}/hardware_irq)
pico_add_subdirectory(${HOST_DIR}/hardware_sync)
pico_add_subdirectory(${HOST_DIR}/hardware_timer)
pico_add_subdirectory(${HOST_DIR}/hardware_uart)
pico_add_subdirectory(${HOST_DIR}/pico_bit_ops)
pico_add_subdirectory(${HOST_DIR}/pico_divider)
pico_add_subdirectory(${HOST_DIR}/pico_multicore)
pico_add_subdirectory(${HOST_DIR}/pico_platform)
pico_add_subdirectory(${HOST_DIR}/pico_rand)
pico_add_subdirectory(${HOST_DIR}/pico_runtime)
pico_add_subdirectory(${HOST_DIR}/pico_printf)
pico_add_subdirectory(${HOST_DIR}/pico_status_led)
pico_add_subdirectory(${HOST_DIR}/pico_stdio)
pico_add_subdirectory(${HOST_DIR}/pico_stdlib)
pico_add_subdirectory(${HOST_DIR}/pico_time_adapter)
pico_add_subdirectory(${HOST_DIR}/pico_unique_id)
