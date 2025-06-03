pico_add_library(pico_status_led)
target_sources(pico_status_led INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/status_led.c
)
target_include_directories(pico_status_led_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
pico_mirrored_target_link_libraries(pico_status_led INTERFACE
    hardware_gpio
    hardware_pio
)
if (PICO_CYW43_SUPPORTED)
    pico_mirrored_target_link_libraries(pico_status_led INTERFACE
        pico_cyw43_driver cyw43_driver_picow pico_async_context_threadsafe_background
    )
    target_compile_definitions(pico_status_led INTERFACE
        CYW43_LWIP_DEFAULT=0 # Disable LwIP by default. Can be overridden if LwIP is needed.
    )
endif()
pico_generate_pio_header(pico_status_led ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

get_target_property(OUT pico_status_led LINK_LIBRARIES)
