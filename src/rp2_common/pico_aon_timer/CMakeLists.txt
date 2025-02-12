pico_add_library(pico_aon_timer)

target_sources(pico_aon_timer INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/aon_timer.c
)

target_include_directories(pico_aon_timer_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_mirrored_target_link_libraries(pico_aon_timer INTERFACE pico_util)

if (TARGET hardware_rtc)
    pico_mirrored_target_link_libraries(pico_aon_timer INTERFACE hardware_rtc)
    target_link_libraries(pico_aon_timer_headers INTERFACE pico_time_headers)
endif()

if (TARGET hardware_powman)
    pico_mirrored_target_link_libraries(pico_aon_timer INTERFACE hardware_powman)
endif()
