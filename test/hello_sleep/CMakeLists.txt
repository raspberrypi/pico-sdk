add_executable(hello_sleep
        hello_sleep.c
        )

target_link_libraries(hello_sleep pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep)


add_executable(hello_sleep_sram1
        hello_sleep.c
        )

target_link_libraries(hello_sleep_sram1 pico_stdlib pico_low_power pico_status_led)

pico_set_persistent_data_loc(hello_sleep_sram1 0x20040000)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_sram1)


add_executable(hello_sleep_xip_sram
        hello_sleep.c
        )

target_link_libraries(hello_sleep_xip_sram pico_stdlib pico_low_power pico_status_led)

pico_set_persistent_data_loc(hello_sleep_xip_sram 0x13ffc000)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_xip_sram)

if (PICO_RP2040)
        add_executable(rtc_clksrc
                rtc_clksrc.c
                )

        target_link_libraries(rtc_clksrc pico_stdlib pico_status_led)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(rtc_clksrc)
endif()

add_executable(hello_sleep_gpio
        hello_sleep_gpio.c
        )

target_link_libraries(hello_sleep_gpio pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_gpio)
