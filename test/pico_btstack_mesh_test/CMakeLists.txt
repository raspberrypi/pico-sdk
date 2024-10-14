cmake_minimum_required(VERSION 3.5)

include(pico_sdk_import.cmake)
project(mesh_node_demo C CXX ASM)

set(PICO_BOARD pico_w)
set(CMAKE_C STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_options(
  -Wall
  -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
  -Wno-maybe-uninitialized
  -O3
  -g
)

add_executable(mesh_node_demo mesh_node_demo.c)

pico_enable_stdio_usb(mesh_node_demo 1)   # Enable USB output
pico_enable_stdio_uart(mesh_node_demo 0)  # Disable UART output
pico_add_extra_outputs(mesh_node_demo)    # Generate map/bin/hex/uf2

target_include_directories(mesh_node_demo
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(mesh_node_demo
  hardware_adc
  pico_btstack_cyw43
  pico_btstack_mesh
  pico_cyw43_arch_lwip_threadsafe_background
  pico_stdlib
)

target_link_options(mesh_node_demo
    PRIVATE "LINKER:--print-memory-usage"
    PRIVATE "LINKER:--verbose"
)
