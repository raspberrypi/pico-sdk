if (NOT TARGET hardware_sha256)
    return()
endif()

pico_add_library(pico_sha256)

target_sources(pico_sha256 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/sha256.c
)

target_include_directories(pico_sha256_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_mirrored_target_link_libraries(pico_sha256 INTERFACE
        hardware_dma
        hardware_sha256
        pico_sync
        )
