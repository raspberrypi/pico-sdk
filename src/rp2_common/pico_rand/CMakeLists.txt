pico_add_impl_library(pico_rand)

target_sources(pico_rand INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/rand.c
)

target_include_directories(pico_rand INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(pico_rand INTERFACE
        pico_unique_id
        hardware_clocks
        hardware_timer
        hardware_sync)
