add_subdirectory(pico_test)

set_directory_properties(PROPERTIES PICO_ALLOW_EXAMPLE_KEYS 1)

add_subdirectory(pico_stdlib_test)
add_subdirectory(pico_stdio_test)
add_subdirectory(pico_time_test)
add_subdirectory(pico_divider_test)
add_subdirectory(fixed_bitset_test)
if (PICO_ON_DEVICE)
    add_subdirectory(panic_function_test)
    add_subdirectory(pico_float_test)
    add_subdirectory(kitchen_sink)
    add_subdirectory(hardware_irq_test)
    add_subdirectory(hardware_pwm_test)
    add_subdirectory(hardware_sync_spin_lock_test)
    add_subdirectory(cmsis_test)
    add_subdirectory(pico_sem_test)
    add_subdirectory(pico_sha256_test)
    add_subdirectory(pico_xip_sram_test)
    add_subdirectory(pico_low_power_test)
    add_subdirectory(pico_async_context_test)
    add_subdirectory(pico_thread_local_test)
endif()

# PICO_TEST_FILE_GENERATOR: Path to the a CMake script to generate test files for ci
if (DEFINED ENV{PICO_TEST_FILE_GENERATOR} AND NOT PICO_TEST_FILE_GENERATOR)
    set(PICO_TEST_FILE_GENERATOR $ENV{PICO_TEST_FILE_GENERATOR})
    message("Using PICO_TEST_FILE_GENERATOR from environment ('${PICO_TEST_FILE_GENERATOR}')")
endif()

if (PICO_TEST_FILE_GENERATOR AND EXISTS ${PICO_TEST_FILE_GENERATOR})
    # Include script file to generate test files for ci
    # This can use the following extra target properties from the test targets:
    # - PICO_TEST_TIMEOUT: Timeout in seconds for the test (default is 5)
    #   Maximum time the test is allowed to run for
    # - PICO_TEST_SUCCESS_STRING: Success string for the test (default is "PASSED")
    #   If this string is printed by the test, the test is considered successful
    # - PICO_TEST_FAILURE_STRING: Failure string for the test (default is "FAILURE_STRING")
    #   Can be used to force failure even when the test prints the success string
    # - PICO_TEST_BUDDY_FILE: File to run on buddy device for the test (default is NONE)
    #   For example, low power tests use external_sleep_timer running on the buddy device
    #   This can either be a path relative to the test target, or an absolute path
    # - PICO_TEST_SKIP_IN_CI: Whether to skip the test in ci (default is FALSE)
    #   If TRUE, the test is not included in the ci test suite
    include(${PICO_TEST_FILE_GENERATOR})
endif()
