pico_add_library(pico_stdio_rtt)

target_sources(pico_stdio_rtt
    INTERFACE
        stdio_rtt.c
        segger_rtt.c
)

target_include_directories(pico_stdio_rtt_headers
    INTERFACE
        ./include
        ../../../lib/rtt/RTT
)

pico_mirrored_target_link_libraries(pico_stdio_rtt INTERFACE pico_stdio)
