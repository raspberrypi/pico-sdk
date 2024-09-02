find_package(Doxygen QUIET)
if (PICO_SDK_TOP_LEVEL_PROJECT AND ${DOXYGEN_FOUND})
    set(PICO_BUILD_DOCS_DEFAULT 1)
endif()
option(PICO_BUILD_DOCS "Build HTML Doxygen docs" ${PICO_BUILD_DOCS_DEFAULT})

if (DEFINED ENV{PICO_EXAMPLES_PATH} AND NOT PICO_EXAMPLES_PATH)
    set(PICO_EXAMPLES_PATH $ENV{PICO_EXAMPLES_PATH})
    message("Using PICO_EXAMPLES_PATH from environment ('${PICO_EXAMPLES_PATH}')")
endif()

if(PICO_BUILD_DOCS)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    include(ExternalProject)

    if(PICO_EXAMPLES_PATH)
        get_filename_component(PICO_EXAMPLES_PATH "${PICO_EXAMPLES_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
        if (EXISTS ${PICO_EXAMPLES_PATH})
            message("Documentation example code will come from ${PICO_EXAMPLES_PATH}")
        else()
            message(WARNING "Documentation example code configured to come from ${PICO_EXAMPLES_PATH}, but that path does not exist")
        endif()
        add_custom_target(doc-pico-examples)
    else()
        ExternalProject_Add(doc-pico-examples
                GIT_REPOSITORY    https://github.com/raspberrypi/pico-examples
                GIT_TAG           master
                CONFIGURE_COMMAND ""
                BUILD_COMMAND ""
                INSTALL_COMMAND ""
                )
        ExternalProject_Get_property(doc-pico-examples SOURCE_DIR)
        ExternalProject_Get_property(doc-pico-examples GIT_REPOSITORY)
        ExternalProject_Get_property(doc-pico-examples GIT_TAG)
        set(PICO_EXAMPLES_PATH ${SOURCE_DIR})
        message("Documentation example code will come from git repo ${GIT_REPOSITORY}, branch ${GIT_TAG}")
    endif()

    set(DOXY_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
    string(REPLACE ";" " " DOXY_INPUT_DIRS "${PICO_DOXYGEN_PATHS}")
    string(REPLACE ";" " " DOXY_EXCLUDE_DIRS "${PICO_DOXYGEN_EXCLUDE_PATHS}")
    string(REPLACE ";" " " DOXY_PREDEFINED "${PICO_DOXYGEN_PRE_DEFINES}")
    string(REPLACE ";" " " DOXY_ENABLED_SECTIONS "${PICO_DOXYGEN_ENABLED_SECTIONS}")
    set(DOXY_EXAMPLE_DIR "${PICO_EXAMPLES_PATH}")
    # auto genereate additional section enables from library paths
    foreach (DIR IN LISTS PICO_DOXYGEN_PATHS)
        get_filename_component(NAME "${DIR}" NAME)
        if (NOT DIR STREQUAL "src")
            set(DOXY_ENABLED_SECTIONS "${DOXY_ENABLED_SECTIONS} ${NAME}")
        endif()
    endforeach ()
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    if (PICO_PLATFORM STREQUAL "rp2040")
        set(PICO_DOXYGEN_TAG "(RP2040)")
    elseif (PICO_PLATFORM STREQUAL "rp2350-arm-s" OR PICO_PLATFORM STREQUAL "rp2350-riscv")
        set(PICO_DOXYGEN_TAG "(RP2350)")
    endif()
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

    add_dependencies(docs doc-pico-examples)
endif()
