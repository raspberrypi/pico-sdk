set(CMAKE_SYSTEM_PROCESSOR hazard3)

set(PICO_DEFAULT_GCC_TRIPLE riscv32-pico-elf riscv32-unknown-elf riscv32-corev-elf riscv-none-elf)

# ordered list of preferred flags to support
set(PICO_COMMON_LANG_FLAGS_LIST
        " -mcpu=hazard3-rp2350"
        " -march=rv32ima_zicsr_zifencei_zba_zbb_zbs_zbkb_zca_zcb_zcmp -mabi=ilp32"
        " -march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb -mabi=ilp32")

# C file used to test the flags listed above. There needs to be an equal number
# of entries in PICO_COMMON_LANG_FLAGS_LIST and PICO_COMMON_LANG_FLAGS_TEST_FILES.
set(PICO_COMMON_LANG_FLAGS_TEST_FILES
        "${CMAKE_CURRENT_LIST_DIR}/util/riscv_zcmp_test.c"
        "${CMAKE_CURRENT_LIST_DIR}/util/riscv_zcmp_test.c"
        "${CMAKE_CURRENT_LIST_DIR}/util/empty.c")

# For context when we look back and wonder why this exists: there was a period where riscv-gnu-toolchain
# shipped a version of GCC with full Zcmp support, but a version of binutils which lacked support for some 
# Zcmp instructions (I think just cm.mvsa01 and cm.mva01s), so the assembler choked on the compiler output 
# if you actually tried to compile anything significant. Hence testing specifically for these instructions
# when checking the supported flags.

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_riscv_gcc_common.cmake)
