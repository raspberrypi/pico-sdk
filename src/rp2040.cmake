# include everything needed to build against rp2040

set(RP2_VARIANT_DIR ${CMAKE_CURRENT_LIST_DIR}/rp2040)
set(PICO_RP2040 "1" CACHE INTERNAL "")
set(PICO_RP2350 "0" CACHE INTERNAL "")
set(PICO_32BIT "1" CACHE INTERNAL "")
set(PICO_RISCV "0" CACHE INTERNAL "")
set(PICO_ARM "1" CACHE INTERNAL "")
set(PICO_CMSIS_DEVICE "RP2040" CACHE INTERNAL "")
set(PICO_DEFAULT_FLASH_SIZE_BYTES "2 * 1024 * 1024")

pico_add_doxygen_pre_define("PICO_RP2040=1")
pico_add_doxygen_pre_define("PICO_RP2350=0")
#pico_add_doxygen_pre_define("NUM_DOORBELLS=0") # we have functions that are gated by this
pico_add_doxygen_enabled_section(rp2040_specific)

include(cmake/rp2_common.cmake)

