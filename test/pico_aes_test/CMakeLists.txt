if (NOT TARGET pico_aes)
    message("Skipping pico_aes_test as pico_aes is unavailable on this platform")
    return()
endif()

add_executable(pico_aes_test
        pico_aes_test.c
        )
target_link_libraries(pico_aes_test
        pico_stdlib
        pico_aes
        pico_rand
)
target_include_directories(pico_aes_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_definitions(pico_aes_test PRIVATE
        PICO_USE_STACK_GUARDS=1
        PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
        PICO_STACK_SIZE=0x9e0 # maximum stack size that fits
)
pico_aes_configure_binary(pico_aes_test)
pico_sign_binary(pico_aes_test ${PICO_SDK_PATH}/tools/example_keys/private.pem)
pico_add_extra_outputs(pico_aes_test)


add_executable(pico_aes_test_single
        pico_aes_test.c
        )
target_link_libraries(pico_aes_test_single
        pico_stdlib
        pico_aes
        pico_rand
)
target_include_directories(pico_aes_test_single PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_definitions(pico_aes_test_single PRIVATE
        PICO_USE_STACK_GUARDS=1
        PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
        PICO_STACK_SIZE=0x9e0 # maximum stack size that fits
)
pico_aes_configure_single_use_binary(pico_aes_test_single)
pico_sign_binary(pico_aes_test_single ${PICO_SDK_PATH}/tools/example_keys/private.pem)
pico_add_extra_outputs(pico_aes_test_single)
