pico_add_impl_library(pico_stdio_rtt)

target_sources(pico_stdio_rtt INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/stdio_rtt.c
        ${CMAKE_CURRENT_LIST_DIR}/../../../lib/SEGGER_RTT/RTT/SEGGER_RTT.c
)

target_include_directories(pico_stdio_rtt INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../../../lib/SEGGER_RTT/RTT
)

target_link_libraries(pico_stdio_rtt INTERFACE pico_stdio)
