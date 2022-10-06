if (DEFINED ENV{PICO_CYW43_DRIVER_PATH} AND (NOT PICO_CYW43_DRIVER_PATH))
    set(PICO_CYW43_DRIVER_PATH $ENV{PICO_CYW43_DRIVER_PATH})
    message("Using PICO_CYW43_DRIVER_PATH from environment ('${PICO_CYW43_DRIVER_PATH}')")
endif()

set(CYW43_DRIVER_TEST_FILE "src/cyw43.h")

# Enable/disable streaming / compression of firmware
if (NOT CYW43_DECOMPRESS_FIRMWARE)
    set(CYW43_DECOMPRESS_FIRMWARE 1)
endif()

if (NOT PICO_CYW43_DRIVER_PATH)
    set(PICO_CYW43_DRIVER_PATH ${PICO_SDK_PATH}/lib/cyw43-driver)
    if (PICO_CYW43_SUPPORTED AND NOT EXISTS ${PICO_CYW43_DRIVER_PATH}/${CYW43_DRIVER_TEST_FILE})
        message(WARNING "cyw43-driver submodule has not been initialized; Pico W wireless support will be unavailable
hint: try 'git submodule update --init' from your SDK directory (${PICO_SDK_PATH}).")
    endif()
elseif (NOT EXISTS ${PICO_CYW43_DRIVER_PATH}/${CYW43_DRIVER_TEST_FILE})
    message(WARNING "PICO_CYW43_DRIVER_PATH specified but content not present.")
endif()

if (EXISTS ${PICO_CYW43_DRIVER_PATH}/${CYW43_DRIVER_TEST_FILE})
    message("cyw43-driver available at ${PICO_CYW43_DRIVER_PATH}")

    pico_register_common_scope_var(PICO_CYW43_DRIVER_PATH)

    # base driver without our bus
    add_library(cyw43_driver_base INTERFACE)
    target_sources(cyw43_driver_base INTERFACE
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_ll.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_stats.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_lwip.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_ctrl.c
            )
    target_include_directories(cyw43_driver_base INTERFACE
            ${PICO_CYW43_DRIVER_PATH}/src
            ${PICO_CYW43_DRIVER_PATH}/firmware
            )
    if (${CYW43_DECOMPRESS_FIRMWARE})
        target_compile_definitions(cyw43_driver_base INTERFACE
            CYW43_DECOMPRESS_FIRMWARE=1
            )
        target_sources(cyw43_driver_base INTERFACE
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_decompress_firmware.c
            )
    endif()
    # Build the driver for cyw43 for pico w

    # Firmware stuff
    if (NOT CYW43_FIRMWARE_LOC)
        set(CYW43_FIRMWARE_LOC ${PICO_CYW43_DRIVER_PATH}/firmware)
    endif()
    if (NOT CYW43_FIRMWARE_BIN)
        set(CYW43_FIRMWARE_BIN "43439A0-7.95.49.00.combined")
        set(CYW43_FIRMWARE_NAME "43439A0_7_95_49_00_combined")
        set(CYW43_FIRMWARE_SYM ${CYW43_FIRMWARE_NAME})
        if (${CYW43_DECOMPRESS_FIRMWARE})
            set(CYW43_FIRMWARE_BIN "${CYW43_FIRMWARE_BIN}.gz")
            set(CYW43_FIRMWARE_SYM "${CYW43_FIRMWARE_SYM}_gz")
        endif()
    endif()
    if (NOT CYW43_FIRMWARE_NAME)
        message(FATAL_ERROR "Please define CYW43_FIRMWARE_NAME")
    endif()
    if (NOT CYW43_FIRMWARE_OBJ)
        # cyw43_resource.o contains the WiFi and BT firmware as a binary blob
        set(CYW43_FIRMWARE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/cyw43_resource.o)
        add_custom_command(
                OUTPUT ${CYW43_FIRMWARE_OBJ}
                DEPENDS ${CYW43_FIRMWARE_LOC}/${CYW43_FIRMWARE_BIN}
                WORKING_DIRECTORY ${CYW43_FIRMWARE_LOC}
                COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm -B arm
                --readonly-text
                --rename-section .data=.big_const,contents,alloc,load,readonly,data
                --redefine-sym _binary_${CYW43_FIRMWARE_SYM}_start=fw_${CYW43_FIRMWARE_NAME}_start
                --redefine-sym _binary_${CYW43_FIRMWARE_SYM}_end=fw_${CYW43_FIRMWARE_NAME}_end
                --redefine-sym _binary_${CYW43_FIRMWARE_SYM}_size=fw_${CYW43_FIRMWARE_NAME}_size
                ${CYW43_FIRMWARE_BIN} ${CYW43_FIRMWARE_OBJ}
        )
    endif()
    add_custom_target(cyw43_firmware_package DEPENDS ${CYW43_FIRMWARE_OBJ})

    add_library(cyw43_driver_picow INTERFACE)
    target_sources(cyw43_driver_picow INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/cyw43_bus_pio_spi.c
            )
    pico_generate_pio_header(cyw43_driver_picow ${CMAKE_CURRENT_LIST_DIR}/cyw43_bus_pio_spi.pio)
    add_dependencies(cyw43_driver_picow INTERFACE cyw43_firmware_package)
    target_link_libraries(cyw43_driver_picow INTERFACE
            ${CYW43_FIRMWARE_OBJ}
            )
    target_include_directories(cyw43_driver_picow INTERFACE
            $(CYW43_FIRMWARE_LOC) # contains a header file with firmware details
            )
    target_link_libraries(cyw43_driver_picow INTERFACE
            cyw43_driver_base
            pico_stdlib
            hardware_pio
            hardware_dma
            hardware_exception
            pico_uzlib
            )
    pico_promote_common_scope_vars()
endif()
