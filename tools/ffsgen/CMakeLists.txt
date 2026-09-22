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


# allow installing to flat dir
include(GNUInstallDirs)
if (FFSGEN_FLAT_INSTALL)
    set(INSTALL_CONFIGDIR ffsgen)
    set(INSTALL_BINDIR ffsgen)
else()
    set(INSTALL_CONFIGDIR lib/cmake/ffsgen)
    set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
endif()

# allow `make install`
install(TARGETS ffsgen
    EXPORT ffsgen-targets
    RUNTIME DESTINATION ${INSTALL_BINDIR}
)

#Export the targets to a script
install(EXPORT ffsgen-targets
    FILE
        ffsgenTargets.cmake
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

include(../../pico_sdk_version.cmake)
set(PACKAGE_VERSION "${PICO_SDK_VERSION_MAJOR}.${PICO_SDK_VERSION_MINOR}.${PICO_SDK_VERSION_REVISION}")

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ffsgenConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY ExactVersion
    ARCH_INDEPENDENT
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/ffsgenConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ffsgenConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

#Install the config and configversion
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ffsgenConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ffsgenConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)
