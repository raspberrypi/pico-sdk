# This is not a platform proper; but is used to build a merged set of documentation

set(PICO_RP2040 "1" CACHE INTERNAL "")
set(PICO_RP2350 "1" CACHE INTERNAL "")
set(PICO_RISCV "1" CACHE INTERNAL "")
set(PICO_ARM "1" CACHE INTERNAL "")
set(PICO_COMBINED_DOCS "1" CACHE INTERNAL "")
# have to pick one for platform stuff, so lets go with rp2350
set(RP2_VARIANT_DIR ${CMAKE_CURRENT_LIST_DIR}/rp2350)
# pick latest version
set(PICO_PIO_VERSION "1" CACHE INTERNAL "")
set(PICO_CMSIS_DEVICE "RP2350" CACHE INTERNAL "")

# Add RP2040 structs too, since there are distinct enums in there
pico_add_doxygen(rp2040/hardware_structs)
# but we DO want dreq.h; it doesn't change much, so lets just use configure_file
# (note we don't add rp2040/hardware_regs because of the size)
configure_file(rp2040/hardware_regs/include/hardware/regs/dreq.h ${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen/dreq_rp2040.h COPYONLY)
# also intctrl.h
configure_file(rp2040/hardware_regs/include/hardware/regs/intctrl.h ${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen/intctrl_rp2040.h COPYONLY)
pico_add_doxygen(${CMAKE_CURRENT_BINARY_DIR}/extra_doxygen)

pico_add_doxygen_pre_define("PICO_RP2040=1")
pico_add_doxygen_pre_define("PICO_RP2350=1")
pico_add_doxygen_pre_define("PICO_COMBINED_DOCS=1")
pico_add_doxygen_pre_define("NUM_DOORBELLS=1") # we have functions that are gated by this
pico_add_doxygen_enabled_section(combined_docs)
pico_add_doxygen_enabled_section(rp2040_specific)
pico_add_doxygen_enabled_section(rp2350_specific)

include(cmake/rp2_common.cmake)

