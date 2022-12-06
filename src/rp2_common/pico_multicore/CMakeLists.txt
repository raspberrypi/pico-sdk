if (NOT TARGET pico_multicore)
    add_library(pico_multicore_headers INTERFACE)
    target_include_directories(pico_multicore_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    pico_add_impl_library(pico_multicore)

    target_sources(pico_multicore INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/multicore.c)


    target_link_libraries(pico_multicore INTERFACE pico_multicore_headers)
    pico_add_header_aware_library_dependency(pico_multicore pico_sync hardware_irq)
endif()



