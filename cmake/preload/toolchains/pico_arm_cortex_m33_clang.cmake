set(CMAKE_SYSTEM_PROCESSOR cortex-m33)

set(PICO_COMMON_LANG_FLAGS "--target=arm-none-eabi -mfloat-abi=hard -march=armv8m.main+fp+dsp")
set(PICO_DISASM_OBJDUMP_ARGS --mcpu=cortex-m33 --arch=armv8m.main+fp+dsp)

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_clang_common.cmake)
