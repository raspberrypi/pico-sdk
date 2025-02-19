# include everything needed to build against rp2350-riscv

set(PICO_RP2040 "0" CACHE INTERNAL "")
set(PICO_RP2350 "1" CACHE INTERNAL "")
set(PICO_32BIT "1" CACHE INTERNAL "")
set(PICO_RISCV "1" CACHE INTERNAL "")
set(PICO_ARM "0" CACHE INTERNAL "")
set(RP2_VARIANT_DIR ${CMAKE_CURRENT_LIST_DIR}/rp2350)
set(PICO_PIO_VERSION "1" CACHE INTERNAL "")
set(PICO_DEFAULT_FLASH_SIZE_BYTES "4 * 1024 * 1024")
# for TinyUSB
set(NO_WARN_RWX_SEGMENTS_SUPPORTED "0" CACHE INTERNAL "")

pico_add_doxygen_pre_define("PICO_RP2040=0")
pico_add_doxygen_pre_define("PICO_RP2350=1")
pico_add_doxygen_enabled_section(rp2350_specific)

include(cmake/rp2_common.cmake)

