add_library(kitchen_sink_libs INTERFACE)

set(KITCHEN_SINK_LIBS
    hardware_adc
    hardware_boot_lock
    hardware_claim
    hardware_clocks
    hardware_dcp
    hardware_divider
    hardware_dma
    hardware_exception
    hardware_flash
    hardware_gpio
    hardware_hazard3
    hardware_i2c
    hardware_interp
    hardware_irq
    hardware_pio
    hardware_pll
    hardware_powman
    hardware_psram
    hardware_pwm
    hardware_rcp
    hardware_resets
    hardware_riscv
    hardware_riscv_platform_timer
    hardware_rtc
    hardware_sha256
    hardware_spi
    hardware_sync
    hardware_sync_spin_lock
    hardware_ticks
    hardware_timer
    hardware_uart
    hardware_vreg
    hardware_watchdog
    hardware_xip_cache
    hardware_xosc
    pico_aon_timer
    pico_bit_ops
    pico_bootrom
    pico_bootsel_via_double_reset
    pico_divider
    pico_double
    pico_fix_rp2040_usb_device_enumeration
    pico_flash
    pico_float
    pico_i2c_slave
    pico_int64_ops
    pico_low_power
    pico_malloc
    pico_mem_ops
    pico_multicore
    pico_platform
    pico_printf
    pico_rand
    pico_runtime
    pico_runtime_init
    pico_sha256
    pico_status_led
    pico_stdio
    pico_stdlib
    pico_sync
    pico_time
    pico_unique_id
    pico_util
)

set(KITCHEN_SINK_NO_HEADER_LIBS
    hardware_dcp
    pico_bootsel_via_double_reset
    pico_int64_ops # currently empty, and only included by _pico variant
    pico_mem_ops # currently empty, and only included by _pico variant
)

set(KITCHEN_SINK_INCLUDES "#pragma once\n")
foreach(LIB IN LISTS KITCHEN_SINK_LIBS)
    if (TARGET ${LIB})
        target_link_libraries(kitchen_sink_libs INTERFACE ${LIB})
        string(REGEX MATCH "([a-z]+)_(.+)" HAS_MATCH ${LIB})
       if (HAS_MATCH AND NOT LIB IN_LIST KITCHEN_SINK_NO_HEADER_LIBS)
           # these are few, so just hack fixing for now
           if (LIB STREQUAL "pico_util")
               string(APPEND KITCHEN_SINK_INCLUDES "#include \"pico/util/datetime.h\"\n")
               string(APPEND KITCHEN_SINK_INCLUDES "#include \"pico/util/pheap.h\"\n")
               string(APPEND KITCHEN_SINK_INCLUDES "#include \"pico/util/queue.h\"\n")
           else()
               if ("${CMAKE_MATCH_2}" STREQUAL "fix_rp2040_usb_device_enumeration")
                   set(CMAKE_MATCH_2 "fix/rp2040_usb_device_enumeration")
               elseif ("${CMAKE_MATCH_2}" STREQUAL "sync_spin_lock")
                   set(CMAKE_MATCH_2 "sync/spin_lock")
               endif()
               string(APPEND KITCHEN_SINK_INCLUDES "#include \"${CMAKE_MATCH_1}/${CMAKE_MATCH_2}.h\"\n")
           endif()
       endif()
    endif()
endforeach ()

if (PICO_VARIANT_DIR)
    # add all hardware_structs headers
    file(GLOB STRUCT_HEADERS "${PICO_VARIANT_DIR}/hardware_structs/include/hardware/structs/*.h")
    foreach(STRUCT_HEADER IN LISTS STRUCT_HEADERS)
        get_filename_component(STRUCT_HEADER ${STRUCT_HEADER} NAME)
        if (PICO_RISCV)
            file(READ ${PICO_VARIANT_DIR}/hardware_structs/include/hardware/structs/${STRUCT_HEADER} STRUCT_HEADER_CONTENT)
            if (STRUCT_HEADER_CONTENT MATCHES "PICO_FORBID_ARM_HEADERS_ON_RISCV")
                #message("Skipping ${STRUCT_HEADER} because it contains PICO_FORBID_ARM_HEADERS_ON_RISCV")
                continue()
            endif()
        endif()
        string(APPEND KITCHEN_SINK_INCLUDES "#include \"hardware/structs/${STRUCT_HEADER}\"\n")
    endforeach ()
endif()

set(KITCHEN_SINK_INCLUDE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kitchen_sink_includes.h")
file(GENERATE OUTPUT ${KITCHEN_SINK_INCLUDE_HEADER} CONTENT ${KITCHEN_SINK_INCLUDES})

add_library(kitchen_sink_options INTERFACE)

if (NOT KITCHEN_SINK_NO_WERROR)
    target_compile_options(kitchen_sink_options INTERFACE
        -Werror
    )
endif()

target_compile_options(kitchen_sink_options INTERFACE
        -Wall
        -Wextra
        #        -pedantic

        -Wnull-dereference
        -Wuninitialized
        -Wunused
        -Wcast-align
        -Wcast-qual
        -Wfloat-equal
        -Wmissing-format-attribute
        #-Wconversion
        -Wsign-compare
        $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
        # todo we have some of these in usb_device_tiny to try to make it more readable.. perhaps doxygen would help here instead
        -Wredundant-decls

        -Wno-inline
        # todo not sure these are true, but investigate
        #-Wpacked

        # todo useful but fix later
        #-Wundef
        )

if (PICO_C_COMPILER_IS_CLANG)
    # todo fix up clang warnings
    target_compile_options(kitchen_sink_options INTERFACE
            -Wno-cast-qual
            -Wno-unused-function
            -Wno-format)
endif()

target_compile_definitions(kitchen_sink_libs INTERFACE
        PICO_FORBID_ARM_HEADERS_ON_RISCV=1
        PARAM_ASSERTIONS_ENABLE_ALL=1 # want to check all the assertions for compilation warnings
        PICO_AUDIO_DMA_IRQ=1
        KITCHEN_SINK_INCLUDE_HEADER="${KITCHEN_SINK_INCLUDE_HEADER}"
)

add_executable(kitchen_sink ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
target_link_libraries(kitchen_sink kitchen_sink_libs kitchen_sink_options)
pico_set_program_name(kitchen_sink "Wombat tentacles")
pico_add_extra_outputs(kitchen_sink)
target_compile_definitions(kitchen_sink PRIVATE KITCHEN_SINK_ID="regular binary")

if (NOT PICO_RP2040)
    add_executable(kitchen_sink_psram_fixed ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_psram_fixed kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_psram_fixed PRIVATE FIXED_PSRAM_SIZE=1)
    if (NOT DEFINED PICO_PSRAM_SIZE_BYTES)
        # Override the PSRAM size for boards that usually don't have PSRAM to allow
        # variables in PSRAM, and auto-detect at runtime if PSRAM is present.
        target_compile_definitions(kitchen_sink_psram_fixed PRIVATE PICO_AUTO_DETECT_PSRAM=1)
        pico_override_psram_size(kitchen_sink_psram_fixed "(1 * 1024 * 1024)")
    endif()
    pico_add_extra_outputs(kitchen_sink_psram_fixed)
    target_compile_definitions(kitchen_sink_psram_fixed PRIVATE KITCHEN_SINK_ID="psram fixed size binary")
    set_target_properties(kitchen_sink_psram_fixed PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")

    add_executable(kitchen_sink_psram_tiny ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_psram_tiny kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_psram_tiny PRIVATE FIXED_PSRAM_SIZE=1 TINY_PSRAM=1)
    # Override the PSRAM size to allow variables in PSRAM
    target_compile_definitions(kitchen_sink_psram_tiny PRIVATE PICO_AUTO_DETECT_PSRAM=1)
    pico_override_psram_size(kitchen_sink_psram_tiny "(1 * 1024 * 1024)")
    pico_add_extra_outputs(kitchen_sink_psram_tiny)
    target_compile_definitions(kitchen_sink_psram_tiny PRIVATE KITCHEN_SINK_ID="psram tiny size binary")
    # Expected to bus fault, so printing PASSED is a failure
    set_target_properties(kitchen_sink_psram_tiny PROPERTIES PICO_TEST_SUCCESS_STRING "psram tiny size binary")
    set_target_properties(kitchen_sink_psram_tiny PROPERTIES PICO_TEST_FAILURE_STRING "PASSED")

    add_executable(kitchen_sink_psram_small ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_psram_small kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_psram_small PRIVATE FIXED_PSRAM_SIZE=1 SMALL_PSRAM=1)
    # Override the PSRAM size to allow variables in PSRAM
    target_compile_definitions(kitchen_sink_psram_small PRIVATE PICO_AUTO_DETECT_PSRAM=1)
    pico_override_psram_size(kitchen_sink_psram_small "(1 * 1024 * 1024)")
    pico_add_extra_outputs(kitchen_sink_psram_small)
    target_compile_definitions(kitchen_sink_psram_small PRIVATE KITCHEN_SINK_ID="psram small size binary")
    set_target_properties(kitchen_sink_psram_small PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")

    add_executable(kitchen_sink_psram_detect ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_psram_detect kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_psram_detect PRIVATE PICO_AUTO_DETECT_PSRAM=1)
    pico_add_extra_outputs(kitchen_sink_psram_detect)
    target_compile_definitions(kitchen_sink_psram_detect PRIVATE KITCHEN_SINK_ID="psram detect size binary")
    set_target_properties(kitchen_sink_psram_detect PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")

    add_executable(kitchen_sink_no_flash_psram_detect ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_no_flash_psram_detect kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_no_flash_psram_detect PRIVATE PICO_AUTO_DETECT_PSRAM=1)
    pico_set_binary_type(kitchen_sink_no_flash_psram_detect no_flash)
    pico_add_extra_outputs(kitchen_sink_no_flash_psram_detect)
    target_compile_definitions(kitchen_sink_no_flash_psram_detect PRIVATE KITCHEN_SINK_ID="psram detect size no_flash binary")
    set_target_properties(kitchen_sink_no_flash_psram_detect PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")

    add_executable(kitchen_sink_psram_flash_fixed ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_psram_flash_fixed kitchen_sink_libs kitchen_sink_options)
    target_compile_definitions(kitchen_sink_psram_flash_fixed PRIVATE FIXED_PSRAM_SIZE=1)
    # Override the FLASH and PSRAM size to check overriding works
    # variables in PSRAM, and auto-detect at runtime if PSRAM is present.
    target_compile_definitions(kitchen_sink_psram_flash_fixed PRIVATE PICO_AUTO_DETECT_PSRAM=1)
    pico_override_flash_size(kitchen_sink_psram_flash_fixed "(1 * 1024 * 1024)")
    pico_override_psram_size(kitchen_sink_psram_flash_fixed "(1 * 1024 * 1024)")
    pico_add_extra_outputs(kitchen_sink_psram_flash_fixed)
    target_compile_definitions(kitchen_sink_psram_flash_fixed PRIVATE KITCHEN_SINK_ID="psram and flash fixed size binary")
    set_target_properties(kitchen_sink_psram_flash_fixed PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
endif()

if (TARGET hardware_pio)
    pico_generate_pio_header(kitchen_sink ${CMAKE_CURRENT_LIST_DIR}/trivial.pio)
endif()

add_executable(kitchen_sink_extra_stdio ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
if (COMMAND suppress_tinyusb_warnings)
    # Explicitly suppress warnings in TinyUSB files which have them (this has to be done
    # from the project that uses them per CMake "feature"). Note the function comes from
    # TinyUSB itself, so we have to guard against TinyUSB not being present with the above if
    suppress_tinyusb_warnings()
endif()
target_compile_definitions(kitchen_sink_extra_stdio PRIVATE
        TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1)
if (TARGET pico_lwip)
    set_source_files_properties(
            ${PICO_LWIP_PATH}/src/core/ipv4/ip4_frag.c
            PROPERTIES
            COMPILE_FLAGS "-Wno-null-dereference")

endif()
target_link_libraries(kitchen_sink_extra_stdio kitchen_sink_libs kitchen_sink_options)
pico_add_extra_outputs(kitchen_sink_extra_stdio)
pico_enable_stdio_usb(kitchen_sink_extra_stdio 1)
pico_enable_stdio_semihosting(kitchen_sink_extra_stdio 1)

add_executable(kitchen_sink_printf_compiler ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
target_link_libraries(kitchen_sink_printf_compiler kitchen_sink_libs kitchen_sink_options)
pico_add_extra_outputs(kitchen_sink_printf_compiler)
pico_set_printf_implementation(kitchen_sink_printf_compiler compiler)

add_executable(kitchen_sink_printf_compiler_no_shortcircuit ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
target_link_libraries(kitchen_sink_printf_compiler_no_shortcircuit kitchen_sink_libs kitchen_sink_options)
pico_add_extra_outputs(kitchen_sink_printf_compiler_no_shortcircuit)
pico_set_printf_implementation(kitchen_sink_printf_compiler_no_shortcircuit compiler)
target_compile_definitions(kitchen_sink_printf_compiler_no_shortcircuit PRIVATE PICO_STDIO_SHORT_CIRCUIT_CLIB_FUNCS=0)


add_executable(kitchen_sink_printf_none ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
target_link_libraries(kitchen_sink_printf_none kitchen_sink_libs kitchen_sink_options)
pico_add_extra_outputs(kitchen_sink_printf_none)
pico_set_printf_implementation(kitchen_sink_printf_none none)
set_target_properties(kitchen_sink_printf_none PROPERTIES PICO_TEST_SUCCESS_STRING "printf support is disabled")

add_executable(kitchen_sink_opaque_absolute_time ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
target_link_libraries(kitchen_sink_opaque_absolute_time kitchen_sink_libs kitchen_sink_options)
pico_add_extra_outputs(kitchen_sink_opaque_absolute_time)
target_compile_definitions(kitchen_sink_opaque_absolute_time PRIVATE KITCHEN_SINK_ID="opaque absolute_time binary" PICO_OPAQUE_ABSOLUTE_TIME_T=1)

if (NOT KITCHEN_SINK_NO_BINARY_TYPE_VARIANTS)
    add_executable(kitchen_sink_copy_to_ram ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    pico_set_binary_type(kitchen_sink_copy_to_ram copy_to_ram)
    target_link_libraries(kitchen_sink_copy_to_ram kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_copy_to_ram)
    target_compile_definitions(kitchen_sink_copy_to_ram PRIVATE KITCHEN_SINK_ID="copy-to-ram binary")

    add_executable(kitchen_sink_no_flash ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    pico_set_binary_type(kitchen_sink_no_flash no_flash)
    target_link_libraries(kitchen_sink_no_flash kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_no_flash)
    target_compile_definitions(kitchen_sink_no_flash PRIVATE KITCHEN_SINK_ID="no-flash binary")

    if (PICO_RP2040)
        # RP2040 has blocked ram
        add_executable(kitchen_sink_blocked_ram ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
        pico_set_binary_type(kitchen_sink_blocked_ram blocked_ram)
        target_link_libraries(kitchen_sink_blocked_ram kitchen_sink_libs kitchen_sink_options)
        pico_add_extra_outputs(kitchen_sink_blocked_ram)
        target_compile_definitions(kitchen_sink_blocked_ram PRIVATE KITCHEN_SINK_ID="blocked-ram binary")
    endif()

    add_executable(kitchen_sink_ram_custom ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    # Have ram start from 0x20020000 length 128k, and heap start from 0x20030000
    pico_set_linker_script_var(kitchen_sink_ram_custom RAM_ORIGIN 0x20020000)
    pico_set_linker_script_var(kitchen_sink_ram_custom RAM_LENGTH 128k)
    pico_set_linker_script_var(kitchen_sink_ram_custom HEAP_LOC 0x20030000)
    # Also swap scratch x and scratch y, to test using defaults
    pico_set_linker_script_var(kitchen_sink_ram_custom SCRATCH_X_ORIGIN SCRATCH_Y_ORIGIN_DEFAULT)
    pico_set_linker_script_var(kitchen_sink_ram_custom SCRATCH_Y_ORIGIN SCRATCH_X_ORIGIN_DEFAULT)
    target_link_libraries(kitchen_sink_ram_custom kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_ram_custom)
    target_compile_definitions(kitchen_sink_ram_custom PRIVATE KITCHEN_SINK_ID="custom ram binary")

    add_executable(kitchen_sink_ram_section ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    pico_add_linker_script_override_path(kitchen_sink_ram_section ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink_ram_section_scripts FILES section_extra_post_data.incl)
    pico_add_linker_script_override_path(kitchen_sink_ram_section ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink_extra_end_scripts FILES section_extra_post_end.incl)
    target_link_libraries(kitchen_sink_ram_section kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_ram_section)
    target_compile_definitions(kitchen_sink_ram_section PRIVATE KITCHEN_SINK_ID="ram section binary" EXTRA_DATA_SECTION=1)

    if (NOT PICO_C_COMPILER_IS_CLANG)
        # Clang does not support overlay sections
        add_executable(kitchen_sink_simple_overlay ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
        pico_add_linker_script_override_path(kitchen_sink_simple_overlay ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink_simple_overlay_scripts FILES section_extra_post_data.incl)
        pico_add_linker_script_override_path(kitchen_sink_simple_overlay ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink_extra_end_scripts  FILES section_extra_post_end.incl)
        target_link_libraries(kitchen_sink_simple_overlay kitchen_sink_libs kitchen_sink_options)
        pico_add_extra_outputs(kitchen_sink_simple_overlay)
        target_compile_definitions(kitchen_sink_simple_overlay PRIVATE KITCHEN_SINK_ID="simple overlay binary" EXTRA_DATA_SECTION=2)
    endif()
endif()

add_executable(kitchen_sink_cpp ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink_cpp.cpp)
target_link_libraries(kitchen_sink_cpp kitchen_sink_libs kitchen_sink_options)
pico_set_program_name(kitchen_sink_cpp "Wombat tentacles CPP")
pico_add_extra_outputs(kitchen_sink_cpp)

if (TARGET pico_cyw43_arch)
    # for lwipopts.h
    add_executable(kitchen_sink_lwip_poll ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_lwip_poll kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_lwip_poll)
    target_link_libraries(kitchen_sink_lwip_poll
            pico_cyw43_arch_lwip_poll)
    if (TARGET pico_btstack)
        target_link_libraries(kitchen_sink_lwip_poll
            pico_btstack_ble pico_btstack_classic pico_btstack_cyw43)
    endif()
    # for lwipopts.h and btstack_config.h.
    target_include_directories(kitchen_sink_lwip_poll PRIVATE
            ${CMAKE_CURRENT_LIST_DIR})

    add_executable(kitchen_sink_lwip_background ${CMAKE_CURRENT_LIST_DIR}/kitchen_sink.c)
    target_link_libraries(kitchen_sink_lwip_background kitchen_sink_libs kitchen_sink_options)
    pico_add_extra_outputs(kitchen_sink_lwip_background)
    target_link_libraries(kitchen_sink_lwip_background
            pico_cyw43_arch_lwip_threadsafe_background)
    # for lwipopts.h
    target_include_directories(kitchen_sink_lwip_background PRIVATE
            ${CMAKE_CURRENT_LIST_DIR})

    if (TARGET pico_lwip_mbedtls)
        target_link_libraries(kitchen_sink_lwip_poll pico_lwip_mbedtls)
        target_link_libraries(kitchen_sink_lwip_background pico_lwip_mbedtls)
    endif()
    if (TARGET pico_mbedtls)
        target_link_libraries(kitchen_sink_lwip_poll pico_mbedtls)
        target_link_libraries(kitchen_sink_lwip_background pico_mbedtls)
        suppress_mbedtls_warnings()
    endif()
endif()

if (TARGET pico_btstack_base AND COMMAND suppress_btstack_warnings)
    suppress_btstack_warnings()
endif()
