# old llvm_libc is missing some functions
if (NOT (PICO_CLIB STREQUAL "llvm_libc" AND CMAKE_C_COMPILER_VERSION VERSION_LESS "21.0.0"))
        add_executable(pico_unaligned_string_test
                pico_unaligned_string_test.c
                )

        target_link_libraries(pico_unaligned_string_test pico_stdlib)

        # Without this the compiler expands or constant-folds most of these calls and
        # the test stops exercising libc, which is the whole point of it.
        target_compile_options(pico_unaligned_string_test PRIVATE -fno-builtin)

        set_target_properties(pico_unaligned_string_test PROPERTIES
                PICO_TEST_SUCCESS_STRING "PASSED"
                PICO_TEST_FAILURE_STRING "FAILURES"
                PICO_TEST_TIMEOUT "30"
                )

        pico_add_extra_outputs(pico_unaligned_string_test)
endif()