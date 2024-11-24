# Finds (or builds) the picotool executable
#
# This will define the following imported targets
#
#     picotool
#
cmake_minimum_required(VERSION 3.17)

if (NOT TARGET picotool)
    include(ExternalProject)

    if (DEFINED ENV{PICOTOOL_FETCH_FROM_GIT_PATH} AND (NOT PICOTOOL_FETCH_FROM_GIT_PATH))
        set(PICOTOOL_FETCH_FROM_GIT_PATH $ENV{PICOTOOL_FETCH_FROM_GIT_PATH})
        message("Using PICOTOOL_FETCH_FROM_GIT_PATH from environment ('${PICOTOOL_FETCH_FROM_GIT_PATH}')")
    endif ()

    include(FetchContent)
    set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
    if (PICOTOOL_FETCH_FROM_GIT_PATH)
        get_filename_component(FETCHCONTENT_BASE_DIR "${PICOTOOL_FETCH_FROM_GIT_PATH}" ABSOLUTE)
    endif ()

    set(picotool_BUILD_TARGET picotoolBuild)
    set(picotool_TARGET picotool)

    if (NOT TARGET ${picotool_BUILD_TARGET})
        if (NOT PICOTOOL_FETCH_FROM_GIT_PATH)
            message(WARNING
                "No installed picotool with version ${picotool_VERSION_REQUIRED} found - building from source\n"
                "It is recommended to build and install picotool separately, or to set PICOTOOL_FETCH_FROM_GIT_PATH "
                "to a common directory for all your SDK projects"
            )
        endif()

        FetchContent_Declare(
                picotool
                GIT_REPOSITORY https://github.com/raspberrypi/picotool.git
                GIT_TAG develop
                GIT_PROGRESS true
        )

        FetchContent_GetProperties(picotool)
        set(picotool_INSTALL_DIR ${FETCHCONTENT_BASE_DIR} CACHE PATH "Directory where picotool has been installed" FORCE)
        if (NOT picotool_POPULATED)
            message("Downloading Picotool")
            FetchContent_Populate(picotool)
        endif ()
        set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})

        add_custom_target(picotoolForceReconfigure
            ${CMAKE_COMMAND} -E touch_nocreate "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
            VERBATIM)

        ExternalProject_Add(${picotool_BUILD_TARGET}
                PREFIX picotool
                SOURCE_DIR ${picotool_SOURCE_DIR}
                BINARY_DIR ${picotool_BINARY_DIR}
                INSTALL_DIR ${picotool_INSTALL_DIR}
                DEPENDS picotoolForceReconfigure
                CMAKE_ARGS
                    "--no-warn-unused-cli"
                    "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                    "-DPICO_SDK_PATH:FILEPATH=${PICO_SDK_PATH}"
                    "-DPICOTOOL_NO_LIBUSB=1"
                    "-DPICOTOOL_FLAT_INSTALL=1"
                    "-DCMAKE_INSTALL_PREFIX=${picotool_INSTALL_DIR}"
                    "-DCMAKE_RULE_MESSAGES=OFF" # quieten the build
                    "-DCMAKE_INSTALL_MESSAGE=NEVER" # quieten the install
                BUILD_ALWAYS 1 # force dependency checking
                EXCLUDE_FROM_ALL TRUE
                TEST_COMMAND
                    ${picotool_INSTALL_DIR}/picotool/picotool
                    version ${picotool_VERSION_REQUIRED}
                TEST_AFTER_INSTALL TRUE
                )
    endif()

    set(picotool_EXECUTABLE ${picotool_INSTALL_DIR}/picotool/picotool)
    add_executable(${picotool_TARGET} IMPORTED GLOBAL)
    set_property(TARGET ${picotool_TARGET} PROPERTY IMPORTED_LOCATION
            ${picotool_EXECUTABLE})

    add_dependencies(${picotool_TARGET} ${picotool_BUILD_TARGET})
endif()
