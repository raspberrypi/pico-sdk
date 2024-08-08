set(CMAKE_SYSTEM_PROCESSOR cortex-m0plus)

# these are all the directories under LLVM embedded toolchain for ARM (newlib or pibolibc) and under llvm_libc
set(PICO_CLANG_RUNTIMES armv6m_soft_nofp armv6m-unknown-none-eabi)

set(PICO_COMMON_LANG_FLAGS "--target=armv6m-none-eabi -mfloat-abi=soft -march=armv6m")

include(${CMAKE_CURRENT_LIST_DIR}/util/pico_arm_clang_common.cmake)
