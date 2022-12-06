add_library(pico_bootrom_headers INTERFACE)
target_include_directories(pico_bootrom_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(pico_bootrom INTERFACE)
target_sources(pico_bootrom INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/bootrom.c
        )

target_link_libraries(pico_bootrom INTERFACE pico_bootrom_headers pico_base_headers)
