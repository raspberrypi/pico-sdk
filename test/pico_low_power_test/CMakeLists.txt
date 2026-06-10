add_library(low_power_test_common INTERFACE)
target_link_libraries(low_power_test_common INTERFACE pico_stdlib pico_low_power pico_status_led)
target_include_directories(low_power_test_common INTERFACE ${CMAKE_CURRENT_LIST_DIR})

add_executable(low_power_test_timers
        low_power_test_timers.c
        )
target_link_libraries(low_power_test_timers low_power_test_common)
set_target_properties(low_power_test_timers PROPERTIES PICO_TEST_TIMEOUT 35)
set_target_properties(low_power_test_timers PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
set_target_properties(low_power_test_timers PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_timers)


add_executable(low_power_test_timers_usb
        low_power_test_timers.c
        )
target_link_libraries(low_power_test_timers_usb low_power_test_common)
pico_enable_stdio_usb(low_power_test_timers_usb 1)
set_target_properties(low_power_test_timers_usb PROPERTIES PICO_TEST_TIMEOUT 35)
set_target_properties(low_power_test_timers_usb PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
set_target_properties(low_power_test_timers_usb PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_timers_usb)


if (NOT PICO_RP2040)
        if (NOT PICO_RISCV)
                add_executable(low_power_test_timers_encrypted
                        low_power_test_timers.c
                        )
                target_link_libraries(low_power_test_timers_encrypted low_power_test_common)
                pico_set_binary_type(low_power_test_timers_encrypted no_flash)
                pico_package_uf2_output(low_power_test_timers_encrypted)
                pico_sign_binary(low_power_test_timers_encrypted ${PICO_SDK_PATH}/tools/example_keys/private.pem)
                pico_encrypt_binary(low_power_test_timers_encrypted ${PICO_SDK_PATH}/tools/example_keys/privateaes.bin ${PICO_SDK_PATH}/tools/example_keys/ivsalt.bin EMBED NO_CLEAR)
                set_target_properties(low_power_test_timers_encrypted PROPERTIES PICO_TEST_TIMEOUT 35)
                set_target_properties(low_power_test_timers_encrypted PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
                set_target_properties(low_power_test_timers_encrypted PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
                # create map/bin/hex file etc.
                pico_add_extra_outputs(low_power_test_timers_encrypted)

                add_executable(low_power_test_timers_encrypted_xip_sram
                        low_power_test_timers.c
                        )
                target_link_libraries(low_power_test_timers_encrypted_xip_sram low_power_test_common)
                pico_set_binary_type(low_power_test_timers_encrypted_xip_sram no_flash)
                pico_package_uf2_output(low_power_test_timers_encrypted_xip_sram)
                pico_sign_binary(low_power_test_timers_encrypted_xip_sram ${PICO_SDK_PATH}/tools/example_keys/private.pem)
                # Encrypted binaries don't clear xip_sram, so NO_CLEAR can be omitted when persistent data is in xip_sram
                pico_encrypt_binary(low_power_test_timers_encrypted_xip_sram ${PICO_SDK_PATH}/tools/example_keys/privateaes.bin ${PICO_SDK_PATH}/tools/example_keys/ivsalt.bin EMBED)
                pico_set_persistent_data_loc(low_power_test_timers_encrypted_xip_sram xip_ram)
                # Also check encrypting with code in xip_sram
                pico_set_time_critical_placement(low_power_test_timers_encrypted_xip_sram xip_ram)
                set_target_properties(low_power_test_timers_encrypted_xip_sram PROPERTIES PICO_TEST_TIMEOUT 35)
                set_target_properties(low_power_test_timers_encrypted_xip_sram PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
                set_target_properties(low_power_test_timers_encrypted_xip_sram PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
                # create map/bin/hex file etc.
                pico_add_extra_outputs(low_power_test_timers_encrypted_xip_sram)
        endif()


        add_executable(low_power_test_timers_sram0
                low_power_test_timers.c
                )
        target_link_libraries(low_power_test_timers_sram0 low_power_test_common)
        pico_set_persistent_data_loc(low_power_test_timers_sram0 0x20020000)
        # create map/bin/hex file etc.
        set_target_properties(low_power_test_timers_sram0 PROPERTIES PICO_TEST_TIMEOUT 35)
        set_target_properties(low_power_test_timers_sram0 PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
        set_target_properties(low_power_test_timers_sram0 PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
        pico_add_extra_outputs(low_power_test_timers_sram0)


        add_executable(low_power_test_timers_sram1
                low_power_test_timers.c
                )
        target_link_libraries(low_power_test_timers_sram1 low_power_test_common)
        pico_set_persistent_data_loc(low_power_test_timers_sram1 0x20040000)
        # create map/bin/hex file etc.
        set_target_properties(low_power_test_timers_sram1 PROPERTIES PICO_TEST_TIMEOUT 35)
        set_target_properties(low_power_test_timers_sram1 PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
        set_target_properties(low_power_test_timers_sram1 PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
        pico_add_extra_outputs(low_power_test_timers_sram1)


        add_executable(low_power_test_timers_xip_sram
                low_power_test_timers.c
                )
        target_link_libraries(low_power_test_timers_xip_sram low_power_test_common)
        pico_set_persistent_data_loc(low_power_test_timers_xip_sram 0x13ffc000)
        # create map/bin/hex file etc.
        set_target_properties(low_power_test_timers_xip_sram PROPERTIES PICO_TEST_TIMEOUT 35)
        set_target_properties(low_power_test_timers_xip_sram PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
        set_target_properties(low_power_test_timers_xip_sram PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
        pico_add_extra_outputs(low_power_test_timers_xip_sram)


        add_executable(low_power_test_timers_xip_sram_no_flash
                low_power_test_timers.c
                )
        target_link_libraries(low_power_test_timers_xip_sram_no_flash low_power_test_common)
        pico_set_persistent_data_loc(low_power_test_timers_xip_sram_no_flash 0x13ffc000)
        pico_set_binary_type(low_power_test_timers_xip_sram_no_flash no_flash)
        pico_package_uf2_output(low_power_test_timers_xip_sram_no_flash)
        # create map/bin/hex file etc.
        set_target_properties(low_power_test_timers_xip_sram_no_flash PROPERTIES PICO_TEST_TIMEOUT 35)
        set_target_properties(low_power_test_timers_xip_sram_no_flash PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
        set_target_properties(low_power_test_timers_xip_sram_no_flash PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
        pico_add_extra_outputs(low_power_test_timers_xip_sram_no_flash)
endif()


add_executable(external_sleep_timer
        external_sleep_timer.c
        )
target_link_libraries(external_sleep_timer low_power_test_common)
# This is a buddy program, not a test
set_target_properties(external_sleep_timer PROPERTIES PICO_TEST_SKIP_IN_CI TRUE)
# create map/bin/hex file etc.
pico_add_extra_outputs(external_sleep_timer)


add_executable(low_power_test_gpio
        low_power_test_gpio.c
        )
target_link_libraries(low_power_test_gpio low_power_test_common)
set_target_properties(low_power_test_gpio PROPERTIES PICO_TEST_TIMEOUT 25)
set_target_properties(low_power_test_gpio PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
set_target_properties(low_power_test_gpio PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_gpio)


add_executable(low_power_test_gpio_usb
        low_power_test_gpio.c
        )
target_link_libraries(low_power_test_gpio_usb low_power_test_common)
pico_enable_stdio_usb(low_power_test_gpio_usb 1)
set_target_properties(low_power_test_gpio_usb PROPERTIES PICO_TEST_TIMEOUT 25)
set_target_properties(low_power_test_gpio_usb PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
set_target_properties(low_power_test_gpio_usb PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_test_gpio_usb)


add_executable(low_power_test_simple
        low_power_test_simple.c
        )
target_link_libraries(low_power_test_simple low_power_test_common)
set_target_properties(low_power_test_simple PROPERTIES PICO_TEST_TIMEOUT 25)
set_target_properties(low_power_test_simple PROPERTIES PICO_TEST_FAILURE_STRING "ERROR:")
set_target_properties(low_power_test_simple PROPERTIES PICO_TEST_BUDDY_FILE external_sleep_timer.elf)
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
