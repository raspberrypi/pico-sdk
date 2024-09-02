# todo there is probably a more "cmake" way of doing this going thru the standard path with our "PICO" platform
#  i.e. CMake<Lang>Information and whatnot

set(CMAKE_SYSTEM_PROCESSOR hazard3)

set(PICO_DEFAULT_GCC_TRIPLE riscv32-unknown-elf riscv32-corev-elf)

set(PICO_COMMON_LANG_FLAGS " -march=rv32ima_zicsr_zifencei_zba_zbb_zbs_zbkb_zca_zcb_zcmp -mabi=ilp32")

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_gcc_common.cmake)
