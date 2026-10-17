# pico_add_sbom(TARGET [NAME name] [VERSION version] [LICENSE spdx-expression] [SUPPLIER supplier]
#               [OUTPUT file] [MANIFEST path...])
# \brief_nodesc\ Generate an CycloneDX Software Bill of Materials for the target after it is linked
#
# Runs tools/sbom/pico_sbom.py as a post-build step. It reads the linker map of the target (so this
# also enables map file output) and lists the SDK, plus every other component with an sbom.json
# manifest, that was linked in. Manifests of components inside the SDK are found automatically; use
# MANIFEST to point at sbom.json files (or directories containing them) for your own components.
#
# \param\ NAME Application name in the SBOM (default: the target name)
# \param\ VERSION Application version
# \param\ LICENSE SPDX license expression of the application
# \param\ SUPPLIER Supplier of the application, e.g. "Organization: Example Ltd"
# \param\ OUTPUT SBOM file to create (default: <target>.cdx.json next to the binary)
# \param\ MANIFEST Extra sbom.json files or directories to search for them
function(pico_add_sbom TARGET)
    cmake_parse_arguments(SBOM "" "NAME;VERSION;LICENSE;SUPPLIER;OUTPUT" "MANIFEST" ${ARGN})
    if (NOT SBOM_NAME)
        set(SBOM_NAME ${TARGET})
    endif()
    if (NOT SBOM_OUTPUT)
        set(SBOM_OUTPUT "$<TARGET_FILE:${TARGET}>.cdx.json")
    endif()
    if (NOT Python3_EXECUTABLE)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    endif()
    get_property(sdk_binary_dir GLOBAL PROPERTY PICO_SDK_BINARY_DIR)

    set(args --map "$<TARGET_FILE:${TARGET}>.map" --sdk-path "${PICO_SDK_PATH}" --name "${SBOM_NAME}"
            --project-dir "${CMAKE_CURRENT_SOURCE_DIR}"
            --path-map "${sdk_binary_dir}" "${PICO_SDK_PATH}"
            --path-map "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    if (SBOM_VERSION)
        list(APPEND args --version "${SBOM_VERSION}")
    endif()
    if (SBOM_LICENSE)
        list(APPEND args --license "${SBOM_LICENSE}")
    endif()
    if (SBOM_SUPPLIER)
        list(APPEND args --supplier "${SBOM_SUPPLIER}")
    endif()
    foreach(manifest ${SBOM_MANIFEST})
        get_filename_component(manifest "${manifest}" ABSOLUTE)
        list(APPEND args --manifest "${manifest}")
    endforeach()

    pico_add_map_output(${TARGET})
    add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${Python3_EXECUTABLE} ${PICO_SDK_PATH}/tools/sbom/pico_sbom.py ${args} --output "${SBOM_OUTPUT}"
            COMMENT "Generating SBOM for ${TARGET}"
            BYPRODUCTS "${SBOM_OUTPUT}"
            VERBATIM)
endfunction()
