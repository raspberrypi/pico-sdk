if (NOT TARGET pico_platform_panic)
    pico_add_library(pico_platform_panic)

    target_sources(pico_platform_panic INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/panic.c
    )

    target_include_directories(pico_platform_panic_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()