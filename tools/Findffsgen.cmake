# Finds (or builds) the ffsgen executable
#
# This will define the following imported targets
#
#     ffsgen
#

if (NOT TARGET ffsgen)
    # todo we would like to use pkg-config to look for it first
    # see https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/

    include(ExternalProject)

    set(FFSGEN_SOURCE_DIR ${PICO_SDK_PATH}/tools/ffsgen)
    set(FFSGEN_BINARY_DIR ${CMAKE_BINARY_DIR}/ffsgen)
    set(FFSGEN_INSTALL_DIR ${CMAKE_BINARY_DIR}/ffsgen-install CACHE PATH "Directory where ffsgen has been installed" FORCE)

    set(ffsgenBuild_TARGET ffsgenBuild)
    set(ffsgen_TARGET ffsgen)

    if (NOT TARGET ${ffsgenBuild_TARGET})
        pico_message_debug("FFSGEN will need to be built")
#        message("Adding external project ${ffsgenBuild_Target} in ${CMAKE_CURRENT_LIST_DIR}}")
        ExternalProject_Add(${ffsgenBuild_TARGET}
                PREFIX ffsgen
                SOURCE_DIR ${FFSGEN_SOURCE_DIR}
                BINARY_DIR ${FFSGEN_BINARY_DIR}
                INSTALL_DIR ${FFSGEN_INSTALL_DIR}
                CMAKE_ARGS
                    "--no-warn-unused-cli"
                    "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                    "-DFFSGEN_FLAT_INSTALL=1"
                    "-DCMAKE_INSTALL_PREFIX=${FFSGEN_INSTALL_DIR}"
                    "-DCMAKE_RULE_MESSAGES=OFF" # quieten the build
                    "-DCMAKE_INSTALL_MESSAGE=NEVER" # quieten the install
                CMAKE_CACHE_ARGS "-DFFSGEN_EXTRA_SOURCE_FILES:STRING=${FFSGEN_EXTRA_SOURCE_FILES}"
                                 "-DFFSGEN_VERSION_STRING:STRING=${PICO_SDK_VERSION_STRING}"
                BUILD_ALWAYS 1 # force dependency checking
                EXCLUDE_FROM_ALL TRUE
                )
    endif()

    if (CMAKE_HOST_WIN32)
        set(ffsgen_EXECUTABLE ${FFSGEN_INSTALL_DIR}/ffsgen/ffsgen.exe)
    else()
        set(ffsgen_EXECUTABLE ${FFSGEN_INSTALL_DIR}/ffsgen/ffsgen)
    endif()
    add_executable(${ffsgen_TARGET} IMPORTED GLOBAL)
    set_property(TARGET ${ffsgen_TARGET} PROPERTY IMPORTED_LOCATION
            ${ffsgen_EXECUTABLE})

    add_dependencies(${ffsgen_TARGET} ${ffsgenBuild_TARGET})
endif()
