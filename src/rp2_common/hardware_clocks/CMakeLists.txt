pico_simple_hardware_target(clocks)

pico_add_header_aware_library_dependency(hardware_clocks
        hardware_gpio
        hardware_irq
        hardware_resets
        hardware_pll
        # not currently used by clocks.c, but sensibly bundled here
        # as changing frequencies may require upping voltage
        hardware_vreg
        hardware_watchdog
        hardware_xosc
)