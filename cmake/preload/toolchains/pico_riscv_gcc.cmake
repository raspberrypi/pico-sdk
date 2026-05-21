set(CMAKE_SYSTEM_PROCESSOR hazard3)

set(PICO_DEFAULT_GCC_TRIPLE riscv32-pico-elf riscv32-unknown-elf riscv32-corev-elf riscv-none-elf)

# ordered list of preferred flags to support
set(PICO_COMMON_LANG_FLAGS_LIST
        " -mcpu=hazard3-rp2350"
        " -march=rv32ima_zicsr_zifencei_zba_zbb_zbs_zbkb_zca_zcb_zcmp -mabi=ilp32"
        " -march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb -mabi=ilp32")
include(${CMAKE_CURRENT_LIST_DIR}/util/pico_riscv_gcc_common.cmake)
