pico_simple_hardware_target(gpio)
pico_mirrored_target_link_libraries(hardware_gpio INTERFACE hardware_irq)

if (PICO_RP2350)
    pico_mirrored_target_link_libraries(hardware_gpio INTERFACE pico_bootrom)
endif()