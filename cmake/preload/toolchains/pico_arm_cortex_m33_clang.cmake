set(CMAKE_SYSTEM_PROCESSOR cortex-m33)

# these are all the directories under LLVM embedded toolchain for ARM (newlib or pibolibc) and under llvm_libc
set(PICO_CLANG_RUNTIMES armv8m.main_soft_nofp armv8m.main-unknown-none-eabi)

set(PICO_COMMON_LANG_FLAGS "-mcpu=cortex-m33 --target=armv8m.main-none-eabi -mfloat-abi=softfp -march=armv8m.main+fp+dsp")
set(PICO_DISASM_OBJDUMP_ARGS --mcpu=cortex-m33 --arch=armv8m.main+fp+dsp)
include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_clang_common.cmake)
