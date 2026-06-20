set(CMAKE_SYSTEM_PROCESSOR cortex-m33)

set(PICO_COMMON_LANG_FLAGS "-mcpu=cortex-m33 --target=armv8m.main-none-eabi -march=armv8m.main+fp+dsp")

set(PICO_DISASM_OBJDUMP_ARGS --mcpu=cortex-m33 --arch=armv8m.main+fp+dsp)

if (PICO_HARD_FLOAT_ABI)
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=hard")
    # todo - doesn't seem to be a hard_fp variant for google atm?
    # these are all the directories under LLVM embedded toolchain for ARM (newlib or picolibc)
    set(PICO_CLANG_RUNTIMES armv8m.main_hard_fp armv8m.main_hard_fp_unaligned armv8m.main_hard_fp_unaligned_size)
else()
    set(PICO_COMMON_LANG_FLAGS "${PICO_COMMON_LANG_FLAGS} -mfloat-abi=softfp")
    # these are all the directories under LLVM embedded toolchain for ARM (newlib or picolibc) and under llvm_libc
    set(PICO_CLANG_RUNTIMES armv8m.main_soft_nofp armv8m.main_soft_nofp_unaligned armv8m.main_soft_nofp_unaligned_size armv8m.main-unknown-none-eabi)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_clang_common.cmake)
