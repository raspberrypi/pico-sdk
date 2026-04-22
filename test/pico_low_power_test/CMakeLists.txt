add_executable(low_power_test_timers
        low_power_test_timers.c
        )

target_link_libraries(low_power_test_timers pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_timers)


add_executable(low_power_test_timers_usb
        low_power_test_timers.c
        )

target_link_libraries(low_power_test_timers_usb pico_stdlib pico_low_power pico_status_led)

pico_enable_stdio_usb(low_power_test_timers_usb 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_timers_usb)


add_executable(low_power_test_timers_sram1
        low_power_test_timers.c
        )

target_link_libraries(low_power_test_timers_sram1 pico_stdlib pico_low_power pico_status_led)

if (NOT PICO_RP2040)
        pico_set_persistent_data_loc(low_power_test_timers_sram1 0x20040000)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_timers_sram1)

if (NOT PICO_RP2040)
        if (NOT PICO_RISCV)
                add_executable(low_power_test_timers_encrypted
                        low_power_test_timers.c
                        )

                target_link_libraries(low_power_test_timers_encrypted pico_stdlib pico_low_power pico_status_led)

                pico_set_binary_type(low_power_test_timers_encrypted no_flash)
                pico_package_uf2_output(low_power_test_timers_encrypted)
                pico_sign_binary(low_power_test_timers_encrypted ${CMAKE_SOURCE_DIR}/tools/example_keys/private.pem)
                pico_encrypt_binary(low_power_test_timers_encrypted ${CMAKE_SOURCE_DIR}/tools/example_keys/privateaes.bin ${CMAKE_SOURCE_DIR}/tools/example_keys/ivsalt.bin EMBED NO_CLEAR)

                # create map/bin/hex file etc.
                pico_add_extra_outputs(low_power_test_timers_encrypted)
        endif()


        add_executable(low_power_test_timers_xip_sram
                low_power_test_timers.c
                )

        target_link_libraries(low_power_test_timers_xip_sram pico_stdlib pico_low_power pico_status_led)

        pico_set_persistent_data_loc(low_power_test_timers_xip_sram 0x13ffc000)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(low_power_test_timers_xip_sram)


        add_executable(low_power_test_timers_xip_sram_no_flash
                low_power_test_timers.c
                )

        target_link_libraries(low_power_test_timers_xip_sram_no_flash pico_stdlib pico_low_power pico_status_led)

        pico_set_persistent_data_loc(low_power_test_timers_xip_sram_no_flash 0x13ffc000)

        pico_set_binary_type(low_power_test_timers_xip_sram_no_flash no_flash)
        pico_package_uf2_output(low_power_test_timers_xip_sram_no_flash)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(low_power_test_timers_xip_sram_no_flash)
endif()

if (PICO_RP2040)
        add_executable(rtc_clksrc
                rtc_clksrc.c
                )

        target_link_libraries(rtc_clksrc pico_stdlib pico_status_led)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(rtc_clksrc)
endif()

add_executable(low_power_test_gpio
        low_power_test_gpio.c
        )

target_link_libraries(low_power_test_gpio pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_gpio)


add_executable(low_power_test_gpio_usb
        low_power_test_gpio.c
        )

target_link_libraries(low_power_test_gpio_usb pico_stdlib pico_low_power pico_status_led)

pico_enable_stdio_usb(low_power_test_gpio_usb 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_gpio_usb)


add_executable(low_power_test_simple
        low_power_test_simple.c
        )

target_link_libraries(low_power_test_simple pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_simple)


add_custom_target(low_power_tests)
get_directory_property(targets BUILDSYSTEM_TARGETS DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "EXECUTABLE")
                add_dependencies(low_power_tests ${target})
        endif()
endforeach()
