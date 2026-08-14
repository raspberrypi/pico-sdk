# These tests are only meaningful when the corresponding check is enabled
if (PICO_UBSAN_ALIGNMENT_CHECKS)
    add_executable(pico_ubsan_align_test
            pico_ubsan_align_test.c
            )

    target_link_libraries(pico_ubsan_align_test pico_stdlib)

    # The point of the test: this file contains a misaligned access and still
    # builds clean under the same warnings kitchen_sink uses, because the
    # pointer is laundered through void *. Only the runtime check finds it.
    target_compile_options(pico_ubsan_align_test PRIVATE
            -Wall
            -Wextra
            -Wcast-align
            -Werror
            )

    # The panic is the pass condition, so tell ci what success looks like.
    set_target_properties(pico_ubsan_align_test PROPERTIES
            PICO_TEST_SUCCESS_STRING "misaligned load"
            PICO_TEST_FAILURE_STRING "FAILED"
            )

    # so the test can tell whether reaching the end is expected
    target_compile_definitions(pico_ubsan_align_test PRIVATE
            PICO_UBSAN_RECOVER=$<BOOL:${PICO_UBSAN_RECOVER}>
            )

    pico_add_extra_outputs(pico_ubsan_align_test)
else()
    message("Skipping pico_ubsan_align_test as PICO_UBSAN_ALIGNMENT_CHECKS is not set")
endif()

if (PICO_UBSAN_NULL_CHECKS)
    add_executable(pico_ubsan_null_test
            pico_ubsan_null_test.c
            )

    target_link_libraries(pico_ubsan_null_test pico_stdlib)

    target_compile_options(pico_ubsan_null_test PRIVATE
            -Wall
            -Wextra
            -Werror
            )

    # The panic is the pass condition, so tell ci what success looks like.
    set_target_properties(pico_ubsan_null_test PROPERTIES
            PICO_TEST_SUCCESS_STRING "null pointer access"
            PICO_TEST_FAILURE_STRING "FAILED"
            )

    # so the test can tell whether reaching the end is expected
    target_compile_definitions(pico_ubsan_null_test PRIVATE
            PICO_UBSAN_RECOVER=$<BOOL:${PICO_UBSAN_RECOVER}>
            )

    pico_add_extra_outputs(pico_ubsan_null_test)
else()
    message("Skipping pico_ubsan_null_test as PICO_UBSAN_NULL_CHECKS is not set")
endif()
