if (NOT TARGET pico_platform_common)
    pico_add_library(pico_platform_common)
    target_sources(pico_platform_common INTERFACE ${CMAKE_CURRENT_LIST_DIR}/common.c)

    target_include_directories(pico_platform_common_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_platform_common_headers INTERFACE hardware_regs)
endif()
