pico_add_library(pico_stdio_rtt)

if (NOT PICO_STDIO_RTT_PATH)
    set(PICO_STDIO_RTT_PATH ${PROJECT_SOURCE_DIR}/lib/SEGGER_RTT/RTT)
endif()

target_sources(pico_stdio_rtt INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/stdio_rtt.c
        ${PICO_STDIO_RTT_PATH}/SEGGER_RTT.c
)

target_include_directories(pico_stdio_rtt_headers INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${PICO_STDIO_RTT_PATH}
)

target_link_libraries(pico_stdio_rtt INTERFACE pico_stdio)


set_source_files_properties(
        ${PICO_STDIO_RTT_PATH}/SEGGER_RTT.c
        PROPERTIES
        COMPILE_OPTIONS
        "-Wno-cast-qual;-Wno-cast-align"
)
