if (NOT TARGET pico_sha256)
    message("Skipping pico_sha256_test as pico_sha256 is unavailable on this platform")
    return()
endif()

add_executable(pico_sha256_test
        pico_sha256_test.c
        )
target_link_libraries(pico_sha256_test
        pico_stdlib
        pico_sha256
)
target_include_directories(pico_sha256_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
pico_add_extra_outputs(pico_sha256_test)
