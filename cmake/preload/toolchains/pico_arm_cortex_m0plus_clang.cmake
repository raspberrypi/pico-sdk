set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)

set(PICO_COMMON_LANG_FLAGS "--target=arm-none-eabi -mcpu=cortex-m0plus -mfloat-abi=soft")

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_clang_common.cmake)
