if (NOT TARGET hardware_dma)
    message("Skipping hardware_irq_test as hardware_dma is unavailable on this platform")
    return()
endif()
add_executable(hardware_irq_test hardware_irq_test.c)

target_link_libraries(hardware_irq_test PRIVATE pico_test hardware_irq hardware_dma)
pico_add_extra_outputs(hardware_irq_test)