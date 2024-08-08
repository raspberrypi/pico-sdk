if (NOT TARGET pico_atomic)
    pico_add_library(pico_atomic)

    target_sources(pico_atomic INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/atomic.c
            )

    target_compile_options(pico_atomic INTERFACE
#            -Wno-atomic-alignment
    )
    target_include_directories(pico_atomic_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_atomic INTERFACE pico_sync)
endif()
