# This file exists for reproducing the RP2350 bootrom build, which depends on the CORE-V development toolchain, as mainline support for Zcmp did not exist at the time.

# Do not use this file for new development. The default toolchain file supports Zcmp.

# todo there is probably a more "cmake" way of doing this going thru the standard path with our "PICO" platform
#  i.e. CMake<Lang>Information and whatnot

set(CMAKE_SYSTEM_PROCESSOR hazard3)

set(PICO_DEFAULT_GCC_TRIPLE riscv32-pico-elf riscv32-unknown-elf riscv32-corev-elf riscv-none-elf)

set(PICO_COMMON_LANG_FLAGS " -march=rv32ima_zicsr_zifencei_zba_zbb_zbs_zbkb_zca_zcb_zcmp -mabi=ilp32")

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_riscv_gcc_common.cmake)
