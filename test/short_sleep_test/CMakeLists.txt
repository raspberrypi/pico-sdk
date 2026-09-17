if (NOT PICO_TIME_NO_ALARM_SUPPORT)
    add_executable(short_sleep_test short_sleep_test.c)
    set_target_properties(short_sleep_test PROPERTIES PICO_TEST_TIMEOUT "20")
    target_compile_definitions(short_sleep_test PRIVATE
            PICO_TIME_SLEEP_OVERHEAD_ADJUST_US=0
            PICO_USE_SW_SPIN_LOCKS=0
    )
    target_link_libraries(short_sleep_test PRIVATE pico_test pico_multicore)
    pico_add_extra_outputs(short_sleep_test)
    pico_set_binary_type(short_sleep_test copy_to_ram)

    if (NOT PICO_RP2040)
        add_executable(short_sleep_test_sw short_sleep_test.c)
        set_target_properties(short_sleep_test_sw PROPERTIES PICO_TEST_TIMEOUT "20")
        target_compile_definitions(short_sleep_test_sw PRIVATE
                PICO_TIME_SLEEP_OVERHEAD_ADJUST_US=0
                PICO_USE_SW_SPIN_LOCKS=1
        )
        target_link_libraries(short_sleep_test_sw PRIVATE pico_test pico_multicore)
        pico_add_extra_outputs(short_sleep_test_sw)
        pico_set_binary_type(short_sleep_test_sw copy_to_ram)
    endif()
endif()