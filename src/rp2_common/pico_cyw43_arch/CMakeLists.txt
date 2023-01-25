if (PICO_CYW43_SUPPORTED) # set by BOARD=pico-w
    if (TARGET cyw43_driver_picow)
        message("Enabling build support for Pico W wireless.")

        pico_add_impl_library(pico_cyw43_arch)
        target_sources(pico_cyw43_arch INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_poll.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_threadsafe_background.c
                ${CMAKE_CURRENT_LIST_DIR}/cyw43_arch_freertos.c
                )

        target_include_directories(pico_cyw43_arch INTERFACE
                ${CMAKE_CURRENT_LIST_DIR}/include)

        target_link_libraries(pico_cyw43_arch INTERFACE
                pico_unique_id
                cyw43_driver_picow)

        if (NOT TARGET pico_lwip)
            message(WARNING "lwIP is not available; Full Pico W wireless support will be unavailable too")
        else()
            add_library(pico_cyw43_arch_lwip_poll INTERFACE)
            target_link_libraries(pico_cyw43_arch_lwip_poll INTERFACE
                    pico_cyw43_arch
                    pico_lwip
                    pico_lwip_nosys)
            target_compile_definitions(pico_cyw43_arch_lwip_poll INTERFACE
                    CYW43_LWIP=1
                    PICO_CYW43_ARCH_POLL=1
                    )

            add_library(pico_cyw43_arch_lwip_threadsafe_background INTERFACE)
            target_link_libraries(pico_cyw43_arch_lwip_threadsafe_background INTERFACE
                    pico_cyw43_arch
                    pico_lwip
                    pico_lwip_nosys)
            target_compile_definitions(pico_cyw43_arch_lwip_threadsafe_background INTERFACE
                    CYW43_LWIP=1
                    PICO_CYW43_ARCH_THREADSAFE_BACKGROUND=1
                    )

            add_library(pico_cyw43_arch_lwip_sys_freertos INTERFACE)
            target_link_libraries(pico_cyw43_arch_lwip_sys_freertos INTERFACE
                    pico_cyw43_arch
                    pico_lwip
                    pico_lwip_contrib_freertos)
            target_compile_definitions(pico_cyw43_arch_lwip_sys_freertos INTERFACE
                    CYW43_LWIP=1
                    LWIP_PROVIDE_ERRNO=1
                    PICO_CYW43_ARCH_FREERTOS=1
                    )
        endif()

        add_library(pico_cyw43_arch_none INTERFACE)
        target_link_libraries(pico_cyw43_arch_none INTERFACE pico_cyw43_arch)
        target_compile_definitions(pico_cyw43_arch_none INTERFACE
                CYW43_LWIP=0
                PICO_CYW43_ARCH_THREADSAFE_BACKGROUND=1 # none still uses threadsafe_background to make gpio use easy
                )
    endif()
endif()

if (PICO_CYW43_DRIVER_PATH AND EXISTS "${PICO_CYW43_DRIVER_PATH}")
    pico_add_doxygen(${PICO_CYW43_DRIVER_PATH}/src)
endif()