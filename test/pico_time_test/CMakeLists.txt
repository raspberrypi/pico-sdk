if (NOT PICO_TIME_NO_ALARM_SUPPORT)
    add_executable(pico_time_test pico_time_test.c)
    set_target_properties(pico_time_test PROPERTIES PICO_TEST_TIMEOUT "20")
    target_compile_definitions(pico_time_test PRIVATE
            PICO_TIME_DEFAULT_ALARM_POOL_MAX_TIMERS=250
            PICO_USE_SW_SPIN_LOCKS=0
    )
    target_link_libraries(pico_time_test PRIVATE pico_test)
    if (PICO_RP2040)
        target_link_libraries(pico_time_test PRIVATE pico_aon_timer)
    endif()
    pico_add_extra_outputs(pico_time_test)

    if (NOT PICO_RP2040)
        add_executable(pico_time_test_sw pico_time_test.c)
        set_target_properties(pico_time_test_sw PROPERTIES PICO_TEST_TIMEOUT "20")
        target_compile_definitions(pico_time_test_sw PRIVATE
                PICO_TIME_DEFAULT_ALARM_POOL_MAX_TIMERS=250
                PICO_USE_SW_SPIN_LOCKS=1
        )
        target_link_libraries(pico_time_test_sw PRIVATE pico_test)
        pico_add_extra_outputs(pico_time_test_sw)
    endif()
endif()