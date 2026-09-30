pico_add_library(pico_ffs)

target_sources(pico_ffs INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ffs.c
)

# The API header is shared with the device implementation.
target_include_directories(pico_ffs_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../rp2_common/pico_ffs/include)
target_link_libraries(pico_ffs_headers INTERFACE pico_base_headers)
