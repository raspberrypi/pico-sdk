if (NOT TARGET pico_ubsan)
    pico_add_library(pico_ubsan)

    target_sources(pico_ubsan INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/ubsan.c
    )

    pico_mirrored_target_link_libraries(pico_ubsan INTERFACE pico_platform_panic)
endif()
