add_executable(hello_sleep
        hello_sleep.c
        )

target_link_libraries(hello_sleep pico_stdlib pico_low_power pico_status_led)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep)


add_executable(hello_sleep_usb
        hello_sleep.c
        )

target_link_libraries(hello_sleep_usb pico_stdlib pico_low_power pico_status_led)

pico_enable_stdio_usb(hello_sleep_usb 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_usb)


add_executable(hello_sleep_sram1
        hello_sleep.c
        )

target_link_libraries(hello_sleep_sram1 pico_stdlib pico_low_power pico_status_led)

if (NOT PICO_RP2040)
        pico_set_persistent_data_loc(hello_sleep_sram1 0x20040000)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_sram1)

if (NOT PICO_RP2040)
        if (NOT PICO_RISCV)
                add_executable(hello_sleep_encrypted
                        hello_sleep.c
                        )

                target_link_libraries(hello_sleep_encrypted pico_stdlib pico_low_power pico_status_led)

                pico_set_binary_type(hello_sleep_encrypted no_flash)
                pico_package_uf2_output(hello_sleep_encrypted)
                pico_sign_binary(hello_sleep_encrypted ${CMAKE_SOURCE_DIR}/tools/example_keys/private.pem)
                pico_encrypt_binary(hello_sleep_encrypted ${CMAKE_SOURCE_DIR}/tools/example_keys/privateaes.bin ${CMAKE_SOURCE_DIR}/tools/example_keys/ivsalt.bin EMBED NO_CLEAR)

                # create map/bin/hex file etc.
                pico_add_extra_outputs(hello_sleep_encrypted)
        endif()


        add_executable(hello_sleep_xip_sram
                hello_sleep.c
                )

        target_link_libraries(hello_sleep_xip_sram pico_stdlib pico_low_power pico_status_led)

        pico_set_persistent_data_loc(hello_sleep_xip_sram 0x13ffc000)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(hello_sleep_xip_sram)


        add_executable(hello_sleep_xip_sram_no_flash
                hello_sleep.c
                )

        target_link_libraries(hello_sleep_xip_sram_no_flash pico_stdlib pico_low_power pico_status_led)

        pico_set_persistent_data_loc(hello_sleep_xip_sram_no_flash 0x13ffc000)

        pico_set_binary_type(hello_sleep_xip_sram_no_flash no_flash)
        pico_package_uf2_output(hello_sleep_xip_sram_no_flash)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(hello_sleep_xip_sram_no_flash)
endif()

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


add_executable(hello_sleep_gpio_usb
        hello_sleep_gpio.c
        )

target_link_libraries(hello_sleep_gpio_usb pico_stdlib pico_low_power pico_status_led)

pico_enable_stdio_usb(hello_sleep_gpio_usb 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_sleep_gpio_usb)


add_custom_target(hello_sleep_examples)
get_directory_property(targets BUILDSYSTEM_TARGETS DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "EXECUTABLE")
                add_dependencies(hello_sleep_examples ${target})
        endif()
endforeach()
