if (NOT PICO_TIME_NO_ALARM_SUPPORT)
    add_executable(pico_stdio_test_uart pico_stdio_test.c)
    target_link_libraries(pico_stdio_test_uart PRIVATE pico_stdlib pico_test pico_multicore)
    pico_add_extra_outputs(pico_stdio_test_uart)
    pico_enable_stdio_uart(pico_stdio_test_uart 1)
    pico_enable_stdio_usb(pico_stdio_test_uart 0)

    add_executable(pico_stdio_test_usb pico_stdio_test.c)
    target_link_libraries(pico_stdio_test_usb PRIVATE pico_stdlib pico_test pico_multicore)
    target_compile_definitions(pico_stdio_test_usb PRIVATE
            PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=-1) # wait for USB connect

    pico_add_extra_outputs(pico_stdio_test_usb)
    pico_enable_stdio_uart(pico_stdio_test_usb 0)
    pico_enable_stdio_usb(pico_stdio_test_usb 1)
endif()