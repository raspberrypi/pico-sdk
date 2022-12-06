pico_simple_hardware_target(irq)

# additional sources/libraries

target_sources(hardware_irq INTERFACE ${CMAKE_CURRENT_LIST_DIR}/irq_handler_chain.S)
pico_add_header_aware_library_dependency(hardware_irq pico_sync)