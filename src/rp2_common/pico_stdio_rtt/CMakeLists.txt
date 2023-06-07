pico_add_library(pico_stdio_rtt)

target_sources(pico_stdio_rtt INTERFACE stdio_rtt.c SEGGER/RTT/SEGGER_RTT.c)

set_source_files_properties(SEGGER/RTT/SEGGER_RTT.c
    PROPERTIES COMPILE_OPTIONS "-Wno-cast-qual;-Wno-cast-align")

target_include_directories(pico_stdio_rtt_headers
    INTERFACE include SEGGER/RTT SEGGER/Config)

pico_mirrored_target_link_libraries(pico_stdio_rtt INTERFACE pico_stdio)

include(CMakePrintHelpers)

cmake_print_properties(
    SOURCES SEGGER/RTT/SEGGER_RTT.c
    PROPERTIES COMPILE_OPTIONS
)
