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
    pico_mirrored_target_link_libraries(pico_cyw43_driver INTERFACE cyw43_driver)

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

    # Set an ip address in a compile definition
    # target name, target type, compile definition name to set then address in a string
    # This can be used to set the following compile definitions
    # CYW43_DEFAULT_IP_ADDRESS
    # CYW43_DEFAULT_IP_MASK
    # CYW43_DEFAULT_IP_GATEWAY
    # CYW43_DEFAULT_IP_DNS
    # CYW43_DEFAULT_AP_IP_ADDRESS
    # e.g. pico_configure_ip4_address(picow_tcpip_server_background PRIVATE CYW43_DEFAULT_IP_ADDRESS "10.3.15.204")
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

    pico_promote_common_scope_vars()
endif()
