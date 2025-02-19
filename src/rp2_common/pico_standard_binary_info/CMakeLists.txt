if (NOT TARGET pico_standard_binary_info)
    pico_add_library(pico_standard_binary_info)

    target_sources(pico_standard_binary_info INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/standard_binary_info.c
    )

    pico_mirrored_target_link_libraries(pico_standard_binary_info INTERFACE pico_binary_info)
endif()