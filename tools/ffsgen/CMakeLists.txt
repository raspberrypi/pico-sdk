cmake_minimum_required(VERSION 3.13)
project(ffsgen C)

set(FFS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../src/rp2_common/pico_ffs)

add_executable(ffsgen
        ffsgen.c
        ${FFS_SRC_DIR}/ffs_internal.c
)

target_include_directories(ffsgen PRIVATE
        ${FFS_SRC_DIR}
)
