pico_add_library(pico_status_led)
target_sources(pico_status_led INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/status_led.c
)
target_include_directories(pico_status_led_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
pico_mirrored_target_link_libraries(pico_status_led INTERFACE
    hardware_gpio
)
