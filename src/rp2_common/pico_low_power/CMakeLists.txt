pico_add_library(pico_low_power)

target_sources(pico_low_power INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/low_power.c
)

target_include_directories(pico_low_power INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_mirrored_target_link_libraries(pico_low_power INTERFACE
        hardware_clocks
        hardware_rosc
        hardware_xosc
        hardware_irq
        hardware_timer
        pico_aon_timer
        )
