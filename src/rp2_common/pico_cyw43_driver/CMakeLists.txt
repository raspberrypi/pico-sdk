# PICO_CMAKE_CONFIG: PICO_CYW43_DRIVER_PATH, Path to cyw43-driver. Can be passed to CMake or set in your environment if you do not wish to use the version included with the SDK, type=string, default=<PICO_SDK_PATH>/lib/cyw43-driver, group=pico_cyw43_driver
if (DEFINED ENV{PICO_CYW43_DRIVER_PATH} AND (NOT PICO_CYW43_DRIVER_PATH))
    set(PICO_CYW43_DRIVER_PATH $ENV{PICO_CYW43_DRIVER_PATH})
    message("Using PICO_CYW43_DRIVER_PATH from environment ('${PICO_CYW43_DRIVER_PATH}')")
endif()

set(CYW43_DRIVER_TEST_FILE "src/cyw43.h")

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

    add_subdirectory(cybt_shared_bus)

    pico_register_common_scope_var(PICO_CYW43_DRIVER_PATH)

    # base driver without our bus
    pico_add_library(cyw43_driver NOFLAG)
    target_sources(cyw43_driver INTERFACE
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_ll.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_stats.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_lwip.c
            ${PICO_CYW43_DRIVER_PATH}/src/cyw43_ctrl.c
            )
    target_include_directories(cyw43_driver_headers SYSTEM INTERFACE
            ${PICO_CYW43_DRIVER_PATH}/src
            ${PICO_CYW43_DRIVER_PATH}/firmware
            )

    # pico_cyw43_driver adds async_context integration to cyw43_driver
    pico_add_library(pico_cyw43_driver NOFLAG)
    target_sources(pico_cyw43_driver INTERFACE
            cyw43_driver.c)
    target_include_directories(pico_cyw43_driver_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    pico_mirrored_target_link_libraries(pico_cyw43_driver INTERFACE cyw43_driver pico_unique_id)

    # cyw43_driver_picow is cyw43_driver plus Pico W specific bus implementation
    pico_add_library(cyw43_driver_picow NOFLAG)
    target_sources(cyw43_driver_picow INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/cyw43_bus_pio_spi.c
            )
    pico_generate_pio_header(cyw43_driver_picow ${CMAKE_CURRENT_LIST_DIR}/cyw43_bus_pio_spi.pio)
    pico_mirrored_target_link_libraries(cyw43_driver_picow INTERFACE
            cyw43_driver
            cybt_shared_bus
            hardware_pio
            hardware_dma
            hardware_exception
            )
    # commented out as I don't think there is a major use case for these to be settable from CMake command line vs board header, or target_compile_defitions
#    if (CYW43_PIN_WL_DYNAMIC)
#        # PICO_CMAKE_CONFIG: CYW43_PIN_WL_DYNAMIC, flag to indicate if cyw43 SPI pins can be changed at runtime, type=bool, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_PIN_WL_DYNAMIC=${CYW43_PIN_WL_DYNAMIC})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_REG_ON)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_REG_ON, gpio pin to power up the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_REG_ON=${CYW43_DEFAULT_PIN_WL_REG_ON})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_DATA_OUT)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_DATA_OUT, gpio pin for spi data out to the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_DATA_OUT=${CYW43_DEFAULT_PIN_WL_DATA_OUT})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_DATA_IN)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_DATA_IN, gpio pin for spi data in from the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_DATA_IN=${CYW43_DEFAULT_PIN_WL_DATA_IN})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_HOST_WAKE)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_HOST_WAKE, gpio (irq) pin for the irq line from the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_HOST_WAKE=${CYW43_DEFAULT_PIN_WL_HOST_WAKE})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_CLOCK)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_CLOCK, gpio pin for the spi clock line to the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_CLOCK=${CYW43_DEFAULT_PIN_WL_CLOCK})
#    endif()
#    if (CYW43_DEFAULT_PIN_WL_CS)
#        # PICO_CMAKE_CONFIG: CYW43_DEFAULT_PIN_WL_CS, gpio pin for the spi chip select to the cyw43 chip, type=int, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_DEFAULT_PIN_WL_CS=${CYW43_DEFAULT_PIN_WL_CS})
#    endif()
#    if (CYW43_PIO_CLOCK_DIV_INT)
#        # PICO_CMAKE_CONFIG: CYW43_PIO_CLOCK_DIV_INT, integer component of pio clock divider used for cyw43 comms, type=int, default=2, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_PIO_CLOCK_DIV_INT=${CYW43_PIO_CLOCK_DIV_INT})
#    endif()
#    if (CYW43_PIO_CLOCK_DIV_FRAC8)
#        # PICO_CMAKE_CONFIG: CYW43_PIO_CLOCK_DIV_FRAC8, fractional component of pio clock divider used for cyw43 comms in range 0-255, type=int, default=0, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_PIO_CLOCK_DIV_FRAC8=${CYW43_PIO_CLOCK_DIV_FRAC8})
#    endif()
#    if (CYW43_PIO_CLOCK_DIV_DYNAMIC)
#        # PICO_CMAKE_CONFIG: CYW43_PIO_CLOCK_DIV_DYNAMIC, flag used to enable dynamic pio clock divider API, type=bool, default=0, group=pico_cyw43_driver
#        target_compile_definitions(cyw43_driver_picow INTERFACE CYW43_PIO_CLOCK_DIV_DYNAMIC=${CYW43_PIO_CLOCK_DIV_DYNAMIC})
#    endif()

    # Note: This is used by MP, so check for issues when making changes
    # e.g. Don't add new depenedences
    pico_add_library(pico_btstack_hci_transport_cyw43 NOFLAG)
    target_sources(pico_btstack_hci_transport_cyw43 INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/btstack_hci_transport_cyw43.c
            ${CMAKE_CURRENT_LIST_DIR}/btstack_chipset_cyw43.c
            )
    target_include_directories(pico_btstack_hci_transport_cyw43_headers SYSTEM INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/include
            )
    target_compile_definitions(pico_btstack_hci_transport_cyw43_headers INTERFACE
            CYW43_ENABLE_BLUETOOTH=1
            )

    if (PICO_CYW43_SUPPORTED AND TARGET pico_btstack_base)
        message("Pico W Bluetooth build support available.")

        pico_add_library(pico_btstack_cyw43)
        target_sources(pico_btstack_cyw43 INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/btstack_cyw43.c
                )
        target_include_directories(pico_btstack_cyw43_headers SYSTEM INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/include
                )
        pico_mirrored_target_link_libraries(pico_btstack_cyw43 INTERFACE
                pico_btstack_base
                pico_btstack_flash_bank
                pico_btstack_run_loop_async_context
                pico_cyw43_arch
                pico_btstack_hci_transport_cyw43
                )
    endif()

    # pico_configure_ip4_address(TARGET_LIB TARGET_TYPE DEF_NAME IP_ADDRESS_STR)
    # \brief\ Set an ip address in a compile definition
    #
    # This can be used to set the following compile definitions;
    # CYW43_DEFAULT_IP_STA_ADDRESS;
    # CYW43_DEFAULT_IP_STA_GATEWAY;
    # CYW43_DEFAULT_IP_AP_ADDRESS;
    # CYW43_DEFAULT_IP_AP_GATEWAY;
    # CYW43_DEFAULT_IP_MASK;
    # CYW43_DEFAULT_IP_DNS;
    # e.g. pico_configure_ip4_address(picow_tcpip_server_background PRIVATE CYW43_DEFAULT_IP_STA_ADDRESS "10.3.15.204")
    #
    # \param\ TARGET_LIB The target library to set the ip address for
    # \param\ TARGET_TYPE The type of target library
    # \param\ DEF_NAME The name of the compile definition to set
    # \param\ IP_ADDRESS_STR The ip address to set
    function(pico_configure_ip4_address TARGET_LIB TARGET_TYPE DEF_NAME IP_ADDRESS_STR)
            string(REGEX MATCHALL "[0-9]+" IP_ADDRESS_LIST ${IP_ADDRESS_STR})
            list(LENGTH IP_ADDRESS_LIST IP_ADDRESS_COMPONENT_COUNT)
            if (NOT ${IP_ADDRESS_COMPONENT_COUNT} EQUAL 4)
                    message(FATAL_ERROR "wrong number of components in ip address 4 != ${IP_ADDRESS_COMPONENT_COUNT}")
            endif()
            set(IP_ADDRESS_HEX "0x0")
            foreach(IP_COMPONENT ${IP_ADDRESS_LIST})
                    if (${IP_COMPONENT} GREATER 255)
                            message(FATAL_ERROR "ip address component too big ${IP_COMPONENT} > 255")
                    endif()
                    math(EXPR IP_ADDRESS_HEX "(${IP_ADDRESS_HEX} << 8 ) | ${IP_COMPONENT}" OUTPUT_FORMAT HEXADECIMAL)
            endforeach()
            target_compile_definitions(${TARGET_LIB} ${TARGET_TYPE}
                    ${DEF_NAME}=${IP_ADDRESS_HEX}
                    )
    endfunction()

    set(PICO_CYW43_DRIVER_CURRENT_PATH ${CMAKE_CURRENT_LIST_DIR})
    pico_register_common_scope_var(PICO_CYW43_DRIVER_CURRENT_PATH)

    pico_promote_common_scope_vars()

    # pico_use_wifi_firmware_partition(TARGET [NO_EMBEDDED_PT])
    # \brief\ Use a partition for the Wi-Fi firmware
    #
    # This will read the CYW43 firmware from a partition with the ID 0x776966696669726d,
    # instead of embedding the firmware blob in the binary. By default it will also embed
    # a compatible partition table in the binary, but this can be disabled by passing the
    # NO_EMBEDDED_PT argument (for example, if you need to chain into the binary, it
    # can't contain a partition table).
    #
    # This will create additional UF2 files for the CYW43 firmware - both a regular version,
    # and a TBYB version to use if you're updating it using the TBYB feature (see section
    # 5.1.17 of the RP2350 datasheet). You will need to flash your chosen version to each
    # new device once, after loading the partition table. For example using picotool:
    #
    # `picotool load TARGET.uf2`;
    # `picotool reboot -u`;
    # `picotool load -ux TARGET_wifi_firmware.uf2`;
    #
    # Then on subsequent updates, you can just flash the TARGET.uf2 file to the device.
    #
    # \param\ NO_EMBEDDED_PT If set, will not embed a partition table in the binary
    function(pico_use_wifi_firmware_partition TARGET)
        set(options NO_EMBEDDED_PT)
        cmake_parse_arguments(PARSE_ARGV 1 OPTS "${options}" "" "")
        if (PICO_PLATFORM STREQUAL "rp2040")
                message(FATAL_ERROR "RP2040 does not support storing wi-fi firmware in partitions")
        endif()
        target_compile_definitions(${TARGET} PRIVATE CYW43_USE_FIRMWARE_PARTITION=1)

        if (NOT OPTS_NO_EMBEDDED_PT)
            get_target_property(picotool_embed_pt ${TARGET} PICOTOOL_EMBED_PT)
            if (NOT picotool_embed_pt)
                pico_embed_pt_in_binary(${TARGET} ${PICO_CYW43_DRIVER_CURRENT_PATH}/wifi_pt.json)
            endif()
        endif()

        find_package (Python3 REQUIRED COMPONENTS Interpreter)

        # CYW43 firmware blob
        add_custom_target(${TARGET}_firmware_blob DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/firmware_blob.S)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/firmware_blob.S
                COMMAND ${Python3_EXECUTABLE} ${PICO_CYW43_DRIVER_CURRENT_PATH}/cyw43_firmware.py ${PICO_CYW43_DRIVER_PATH}/firmware/wb43439A0_7_95_49_00_combined.h ${CMAKE_CURRENT_BINARY_DIR}/firmware_blob.S
                )

        # Create UF2s for regular and TBYB firmwares
        add_executable(${TARGET}_wifi_firmware
                ${PICO_CYW43_DRIVER_CURRENT_PATH}/wifi_firmware.S)
        add_executable(${TARGET}_wifi_firmware_tbyb
                ${PICO_CYW43_DRIVER_CURRENT_PATH}/wifi_firmware.S)

        add_dependencies(${TARGET}_wifi_firmware ${TARGET}_firmware_blob)
        add_dependencies(${TARGET}_wifi_firmware_tbyb ${TARGET}_firmware_blob)

        target_include_directories(${TARGET}_wifi_firmware PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
        target_include_directories(${TARGET}_wifi_firmware_tbyb PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

        target_compile_definitions(${TARGET}_wifi_firmware PRIVATE
                NO_PICO_PLATFORM=1
                )
        target_compile_definitions(${TARGET}_wifi_firmware_tbyb PRIVATE
                NO_PICO_PLATFORM=1
                PICO_CRT0_IMAGE_TYPE_TBYB=1
                )

        target_link_options(${TARGET}_wifi_firmware PRIVATE -nostartfiles -nodefaultlibs LINKER:--script=${PICO_CYW43_DRIVER_CURRENT_PATH}/wifi_firmware.ld)
        target_link_options(${TARGET}_wifi_firmware_tbyb PRIVATE -nostartfiles -nodefaultlibs LINKER:--script=${PICO_CYW43_DRIVER_CURRENT_PATH}/wifi_firmware.ld)

        target_link_libraries(${TARGET}_wifi_firmware boot_picobin_headers)
        target_link_libraries(${TARGET}_wifi_firmware_tbyb boot_picobin_headers)

        get_target_property(hasSigfile ${TARGET} PICOTOOL_SIGFILE)
        if (hasSigfile)
                pico_sign_binary(${TARGET}_wifi_firmware ${sigfile})
                pico_sign_binary(${TARGET}_wifi_firmware_tbyb ${sigfile})
        endif()

        pico_hash_binary(${TARGET}_wifi_firmware)
        pico_hash_binary(${TARGET}_wifi_firmware_tbyb)

        pico_set_uf2_family(${TARGET}_wifi_firmware cyw43-firmware)
        pico_set_uf2_family(${TARGET}_wifi_firmware_tbyb cyw43-firmware)

        pico_package_uf2_output(${TARGET}_wifi_firmware 0x10000000)
        pico_package_uf2_output(${TARGET}_wifi_firmware_tbyb 0x10000000)

        pico_add_extra_outputs(${TARGET}_wifi_firmware)
        pico_add_extra_outputs(${TARGET}_wifi_firmware_tbyb)

        add_dependencies(${TARGET}
                ${TARGET}_wifi_firmware ${TARGET}_wifi_firmware_tbyb)
    endfunction()
endif()
