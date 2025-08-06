# don't use pico_add_library here as picotool includes it directly
add_library(pico_usb_reset_interface_headers INTERFACE)

target_include_directories(pico_usb_reset_interface_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
