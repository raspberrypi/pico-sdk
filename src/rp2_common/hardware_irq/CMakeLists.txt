pico_simple_hardware_target(irq)

# additional sources/libraries

target_sources(hardware_irq INTERFACE ${CMAKE_CURRENT_LIST_DIR}/irq_handler_chain.S)

target_link_libraries(hardware_irq INTERFACE pico_sync) # not mirrored as only implementation requires it
if (PICO_RISCV)
    pico_mirrored_target_link_libraries(hardware_irq INTERFACE hardware_hazard3)
endif()
