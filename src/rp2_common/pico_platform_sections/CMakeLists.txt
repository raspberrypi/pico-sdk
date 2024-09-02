if (NOT TARGET pico_platform_sections)
    pico_add_library(pico_platform_sections)

    target_include_directories(pico_platform_sections_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
endif()
