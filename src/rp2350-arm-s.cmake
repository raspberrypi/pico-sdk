# include everything needed to build against rp2350

set(PICO_RP2040 "0" CACHE INTERNAL "")
set(PICO_RP2350 "1" CACHE INTERNAL "")
set(PICO_32BIT "1" CACHE INTERNAL "")
set(PICO_RISCV "0" CACHE INTERNAL "")
set(PICO_ARM "1" CACHE INTERNAL "")
set(RP2_VARIANT_DIR ${CMAKE_CURRENT_LIST_DIR}/rp2350)
set(PICO_PIO_VERSION "1" CACHE INTERNAL "")
set(PICO_CMSIS_DEVICE "RP2350" CACHE INTERNAL "")
set(PICO_DEFAULT_FLASH_SIZE_BYTES "4 * 1024 * 1024")

pico_add_doxygen_pre_define("PICO_RP2040=0")
pico_add_doxygen_pre_define("PICO_RP2350=1")
pico_add_doxygen_pre_define("NUM_DOORBELLS=1") # we have functions that are gated by this
pico_add_doxygen_enabled_section(rp2350_specific)

# for now we are building RISC-V into RP2350 docs, so document these too
pico_add_doxygen(rp2_common/hardware_riscv)
pico_add_doxygen(rp2_common/hardware_hazard3)

include(cmake/rp2_common.cmake)

