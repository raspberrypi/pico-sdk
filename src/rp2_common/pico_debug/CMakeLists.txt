pico_add_impl_library(pico_debug)

target_sources(pico_debug INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/print_exception.c
        ${CMAKE_CURRENT_LIST_DIR}/debug_exception.S
)

target_link_libraries(pico_debug INTERFACE
        pico_printf
        )
