if (DEFINED ENV{PICO_TINYUSB_PATH} AND (NOT PICO_TINYUSB_PATH))
    set(PICO_TINYUSB_PATH $ENV{PICO_TINYUSB_PATH})
    message("Using PICO_TINYUSB_PATH from environment ('${PICO_TINYUSB_PATH}')")
endif ()

set(TINYUSB_TEST_PATH "src/portable/raspberrypi/rp2040")
if (NOT PICO_TINYUSB_PATH)
    set(PICO_TINYUSB_PATH ${PROJECT_SOURCE_DIR}/lib/tinyusb)
    if (NOT EXISTS ${PICO_TINYUSB_PATH}/${TINYUSB_TEST_PATH})
        message(WARNING "TinyUSB submodule has not been initialized; USB support will be unavailable
hint: try 'git submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_TINYUSB_PATH}/${TINYUSB_TEST_PATH})
    message(WARNING "PICO_TINYUSB_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_TINYUSB_PATH}/${TINYUSB_TEST_PATH})
    message("TinyUSB available at ${PICO_TINYUSB_PATH}/${TINYUSB_TEST_PATH}; enabling build support for USB.")

    pico_register_common_scope_var(PICO_TINYUSB_PATH)

    set(BOARD pico_sdk)
    set(FAMILY rp2040)
    include(${PICO_TINYUSB_PATH}/hw/bsp/family_support.cmake)

    function(add_tinyusb_library RTOS)
        if (RTOS STREQUAL noos)
            set(RTOS pico)
            set(SUFFIX "")
        else()
            set(SUFFIX _${RTOS})
        endif()
        string(TOUPPER ${RTOS} RTOS_UPPER)

        add_library(tinyusb_common${SUFFIX} INTERFACE)
        target_compile_definitions(tinyusb_common${SUFFIX} INTERFACE CFG_TUSB_OS=OPT_OS_${RTOS_UPPER})
        target_link_libraries(tinyusb_common${SUFFIX} INTERFACE tinyusb_common_base)

        add_library(tinyusb_device_unmarked${SUFFIX} INTERFACE)
        target_link_libraries(tinyusb_device_unmarked${SUFFIX} INTERFACE tinyusb_device_base)
        target_compile_definitions(tinyusb_device_unmarked${SUFFIX} INTERFACE
            # off by default note TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX defaults from PICO_RP2040_USB_DEVICE_ENUMERATION_FIX
            # TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1
            PICO_RP2040_USB_DEVICE_UFRAME_FIX=1
        )

        # unmarked version used by stdio USB
        target_link_libraries(tinyusb_device_unmarked${SUFFIX} INTERFACE tinyusb_common${SUFFIX} pico_fix_rp2040_usb_device_enumeration tinyusb_device_base)

        pico_add_library(tinyusb_device${SUFFIX})
        target_link_libraries(tinyusb_device${SUFFIX} INTERFACE tinyusb_device_unmarked${SUFFIX})

        pico_add_library(tinyusb_host${SUFFIX})
        target_link_libraries(tinyusb_host${SUFFIX} INTERFACE tinyusb_host_base tinyusb_common${SUFFIX})

        pico_add_library(tinyusb_board${SUFFIX})
        target_link_libraries(tinyusb_board${SUFFIX} INTERFACE tinyusb_bsp)
    endfunction()

    # no RTOS: tinyusb_device, tinyusb_host
    add_tinyusb_library(noos)

    # FreeRTOS: tinyusb_device_freertos, tinyusb_host_freertos
    add_tinyusb_library(freertos)
    #target_link_libraries(tinyusb_common_freertos INTERFACE freertos_kernel)

    # Override suppress_tinyusb_warnings to add suppression of (falsely) reported GCC 11.2 warnings
    function(suppress_tinyusb_warnings)
        _suppress_tinyusb_warnings()
        if (PICO_C_COMPILER_IS_GNU)
            set_source_files_properties(
                    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/rp2040/rp2040_usb.c
                    PROPERTIES
                    COMPILE_FLAGS "-Wno-stringop-overflow -Wno-array-bounds")
        endif()
    endfunction()

    pico_promote_common_scope_vars()
endif()
