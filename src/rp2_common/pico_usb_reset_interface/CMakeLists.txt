if (TARGET tinyusb_device_unmarked)
    pico_add_impl_library(pico_usb_reset_interface)

    target_sources(pico_usb_reset_interface INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/usb_reset_interface.c
    )

    target_link_libraries(pico_usb_reset_interface INTERFACE
        tinyusb_device_unmarked
    )
endif()
