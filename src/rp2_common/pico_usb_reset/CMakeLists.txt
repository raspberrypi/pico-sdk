if (TARGET tinyusb_device_unmarked)
    pico_add_library(pico_usb_reset)

    target_sources(pico_usb_reset INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/usb_reset.c
    )

    target_include_directories(pico_usb_reset_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_link_libraries(pico_usb_reset INTERFACE tinyusb_device_unmarked)

    target_link_libraries(pico_usb_reset_headers INTERFACE pico_usb_reset_interface_headers)
endif()
