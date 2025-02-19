pico_add_library(pico_stdio_rtt)

target_sources(pico_stdio_rtt INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/stdio_rtt.c
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/RTT/SEGGER_RTT.c)

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/SEGGER/RTT/SEGGER_RTT.c
    PROPERTIES COMPILE_OPTIONS "-Wno-cast-qual;-Wno-cast-align")

target_include_directories(pico_stdio_rtt_headers INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/include 
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/RTT 
    ${CMAKE_CURRENT_LIST_DIR}/SEGGER/Config)

pico_mirrored_target_link_libraries(pico_stdio_rtt INTERFACE pico_stdio)
