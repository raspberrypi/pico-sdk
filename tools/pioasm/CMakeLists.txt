cmake_minimum_required(VERSION 3.13...3.27)
project(pioasm CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 11)

if (PIOASM_GENERATE_PARSER)
    find_package(BISON 3.4.2)
    find_package(FLEX 2.5.13) # no idea about the version

    FLEX_TARGET(pioasm_lexer lexer.ll ${CMAKE_CURRENT_SOURCE_DIR}/gen/lexer.cpp)
    BISON_TARGET(pioasm_parser parser.yy ${CMAKE_CURRENT_SOURCE_DIR}/gen/parser.cpp COMPILE_FLAGS "-Wcounterexamples")
    ADD_FLEX_BISON_DEPENDENCY(pioasm_lexer pioasm_parser)
endif()

add_executable(pioasm
        main.cpp
        pio_assembler.cpp
        pio_disassembler.cpp
        gen/lexer.cpp
        gen/parser.cpp
)

target_sources(pioasm PRIVATE c_sdk_output.cpp)
target_sources(pioasm PRIVATE python_output.cpp)
target_sources(pioasm PRIVATE hex_output.cpp)
target_sources(pioasm PRIVATE json_output.cpp)
target_sources(pioasm PRIVATE ada_output.cpp)
target_sources(pioasm PRIVATE go_output.cpp)
target_sources(pioasm PRIVATE ${PIOASM_EXTRA_SOURCE_FILES})
target_sources(pioasm PRIVATE pio_types.h)

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7") AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9") AND
    (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm.*$"))
    # disable GCC ARM info notice about ABI change
    target_compile_options(pioasm PRIVATE -Wno-psabi)
endif()

target_include_directories(pioasm PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/gen)

if (MSVC OR
    (WIN32 AND NOT MINGW AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    target_compile_definitions(pioasm PRIVATE YY_NO_UNISTD_H)
endif()
if (MSVC)
    target_compile_options(pioasm PRIVATE "/std:c++latest")
endif()


# allow installing to flat dir
include(GNUInstallDirs)
if (PIOASM_FLAT_INSTALL)
    set(INSTALL_CONFIGDIR pioasm)
    set(INSTALL_BINDIR pioasm)
else()
    set(INSTALL_CONFIGDIR lib/cmake/pioasm)
    set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
endif()

# allow `make install`
install(TARGETS pioasm
    EXPORT pioasm-targets
    RUNTIME DESTINATION ${INSTALL_BINDIR}
)

#Export the targets to a script
install(EXPORT pioasm-targets
    FILE
        pioasmTargets.cmake
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

include(../../pico_sdk_version.cmake)
set(PACKAGE_VERSION "${PICO_SDK_VERSION_MAJOR}.${PICO_SDK_VERSION_MINOR}.${PICO_SDK_VERSION_REVISION}")

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pioasmConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY ExactVersion
    ARCH_INDEPENDENT
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/pioasmConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/pioasmConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config and configversion
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pioasmConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/pioasmConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)
