set(CMAKE_DIR cmake)
set(COMMON_DIR common)
set(HOST_DIR host)

# include everything needed to build against rp2040

set(PICO_RP2040 "1" CACHE INTERNAL "")
set(PICO_RP2350 "0" CACHE INTERNAL "")
set(PICO_RISCV "0" CACHE INTERNAL "")
set(PICO_ARM "0" CACHE INTERNAL "")
set(RP2_VARIANT_DIR ${CMAKE_CURRENT_LIST_DIR}/rp2040)
set(PICO_CMSIS_DEVICE "RP2040" CACHE INTERNAL "")
set(PICO_DEFAULT_FLASH_SIZE_BYTES "2 * 1024 * 1024")

include (${CMAKE_DIR}/no_hardware.cmake)

unset(CMAKE_DIR)
unset(COMMON_DIR)
unset(HOST_DIR)