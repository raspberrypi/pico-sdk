if (TARGET pico_async_context_threadsafe_background)
    add_executable(pico_async_context_test
        pico_async_context_test.c
    )
    target_link_libraries(pico_async_context_test PRIVATE
        pico_test
        pico_async_context_threadsafe_background
    )
    pico_add_extra_outputs(pico_async_context_test)
endif()
