if (NOT TARGET pico_standard_link)
    pico_add_library(pico_standard_link)

    if (TARGET boot_stage2_headers)
        target_link_libraries(pico_standard_link INTERFACE boot_stage2_headers)
    endif()

    pico_register_common_scope_var(PICO_LINKER_SCRIPT_INCLUDE_DIRS)

    # add the default linker script include directories
    list(APPEND PICO_LINKER_SCRIPT_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/script_include)

    # pico_add_link_depend(TARGET dependency)
    # \brief\ Add a link time dependency to the target
    #
    # \param\ dependency The dependency to add
    function(pico_add_link_depend TARGET dependency)
        get_target_property(target_type ${TARGET} TYPE)
        if (${target_type} STREQUAL "INTERFACE_LIBRARY")
            set(PROP "INTERFACE_LINK_DEPENDS")
        else()
            set(PROP "LINK_DEPENDS")
        endif()
        get_target_property(_LINK_DEPENDS ${TARGET} ${PROP})
        if (NOT _LINK_DEPENDS)
            set(_LINK_DEPENDS ${dependency})
        else()
            list(APPEND _LINK_DEPENDS ${dependency})
        endif()
        set_target_properties(${TARGET} PROPERTIES ${PROP} "${_LINK_DEPENDS}")
    endfunction()

    
    # pico_add_memmap_link_depends(TARGET TYPE)
    # \brief\ Add link dependencies for the binary type to the target
    #
    # \param\ TYPE The binary type to use
    function(pico_add_memmap_link_depends TARGET TYPE)
        include(${PICO_LINKER_SCRIPT_PATH}/memmap_${TYPE}.cmake)
        pico_add_memmap_link_depends_internal(${TARGET})
    endfunction()

    # pico_check_linker_script(LDSCRIPT)
    # \brief_nodesc\ Check the linker script for compatibility
    #
    # Checks the linker script for compatibility with the current SDK version,
    # and if not, raises warnings and enables workarounds to maintain
    # compatibility where possible.
    #
    # \param\ LDSCRIPT Full path to the linker script to check
    function(pico_check_linker_script TARGET LDSCRIPT)
        if (EXISTS ${LDSCRIPT})
            file(READ ${LDSCRIPT} LDSCRIPT_CONTENTS)
        else()
            return()
        endif()

        # Check if the linker script uses KEEP to keep the .stack and .heap sections
        # and if not, set PICO_CRT0_ALLOCATE_SPACERS to 0 to maintain compatibility
        string(FIND "${LDSCRIPT_CONTENTS}" "KEEP(*(.stack*))" KEEP_STACK_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "KEEP(*(.heap*))" KEEP_HEAP_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "*(.stack*)" STACK_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "*(.heap*)" HEAP_FOUND)
        set(PICO_CRT0_ALLOCATE_SPACERS TRUE)
        if ((${STACK_FOUND} GREATER -1) AND NOT (${KEEP_STACK_FOUND} GREATER -1))
            message(WARNING "Linker script ${LDSCRIPT} does not KEEP the .stack section - replace `*(.stack*)` with `KEEP(*(.stack*))`")
            set(PICO_CRT0_ALLOCATE_SPACERS FALSE)
        endif()
        if ((${HEAP_FOUND} GREATER -1) AND NOT (${KEEP_HEAP_FOUND} GREATER -1))
            message(WARNING "Linker script ${LDSCRIPT} does not KEEP the .heap section - replace `*(.heap*)` with `KEEP(*(.heap*))`")
            set(PICO_CRT0_ALLOCATE_SPACERS FALSE)
        endif()
        if (NOT ${PICO_CRT0_ALLOCATE_SPACERS})
            message(WARNING "Linker script ${LDSCRIPT} is incompatible with certain Pico SDK >2.1.1 features; setting PICO_CRT0_ALLOCATE_SPACERS=0 as a workaround")
            target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_ALLOCATE_SPACERS=0)
        endif()
    endfunction()

    # pico_set_linker_script(TARGET LDSCRIPT)
    # \brief\ Set the linker script for the target
    #
    # \param\ LDSCRIPT Full path to the linker script to set
    function(pico_set_linker_script TARGET LDSCRIPT)
        pico_check_linker_script(${TARGET} ${LDSCRIPT})
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_LINKER_SCRIPT ${LDSCRIPT})
    endfunction()

    # pico_set_linker_script_var(TARGET NAME VALUE)
    # \brief\ Set the linker script for the target
    #
    # \param\ NAME Name of variable to set
    # \param\ VALUE Value of variable to set
    function(pico_set_linker_script_var TARGET NAME VALUE)
        set_property(TARGET ${TARGET} APPEND PROPERTY PICO_TARGET_LINKER_SCRIPT_VARS "${NAME}=${VALUE}")
    endfunction()

    # pico_add_linker_script_override_path(TARGET PATH [FILES <files...>] [GLOB_FILES])
    # \brief\ Add an override linker script path to the target
    #
    # This can be used to override default linker script files with custom versions.
    #
    # For example, to use custom files in ${CMAKE_CURRENT_LIST_DIR}/extra_scripts instead of the default ones,
    # call pico_add_linker_script_override_path(TARGET ${CMAKE_CURRENT_LIST_DIR}/extra_scripts). This will
    # include the custom files first, overriding the default ones.
    #
    # It will not override prior paths set by pico_add_linker_script_override_path.
    #
    # \param\ TARGET The target to add the linker script override path to
    # \param\ PATH The path containing the overriding linker scripts
    # \param\ FILES The files in PATH - this is only used to calculate dependencies
    # \param\ GLOB_FILES Instead of passing FILES, just glob for all .incl files in directory
    function(pico_add_linker_script_override_path TARGET PATH)
        set(options GLOB_FILES)
        set(multiValueArgs FILES)
        cmake_parse_arguments(PARSE_ARGV 2 DEP "${options}" "${oneValueArgs}" "${multiValueArgs}")
        set_property(TARGET ${TARGET} APPEND PROPERTY PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS "${PATH}")
        if (DEP_GLOB_FILES)
            file(GLOB scripts CONFIGURE_DEPENDS "${PATH}/*.incl")
            foreach(script ${scripts})
                pico_add_link_depend(${TARGET} ${script})
            endforeach()
        else()
            foreach(script ${DEP_FILES})
                pico_add_link_depend(${TARGET} ${PATH}/${script})
            endforeach()
        endif()
    endfunction()

    # pico_override_flash_size(TARGET FLASH_SIZE)
    # \brief\ Override the FLASH size for a given target
    #
    # \param\ TARGET The target to override the FLASH size for
    # \param\ FLASH_SIZE The FLASH size to set for the target
    function(pico_override_flash_size TARGET FLASH_SIZE)
        set(PICO_FLASH_SIZE_BYTES_STRING "${FLASH_SIZE}")
        configure_file(${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_flash_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/pico_flash_region.ld)
        pico_add_linker_script_override_path(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} FILES pico_flash_region.ld)
        target_compile_definitions(${TARGET} PRIVATE "PICO_FLASH_SIZE_BYTES=${FLASH_SIZE}")
    endfunction()

    # pico_override_psram_size(TARGET PSRAM_SIZE)
    # \brief\ Override the PSRAM size for a given target
    #
    # \param\ TARGET The target to override the PSRAM size for
    # \param\ PSRAM_SIZE The PSRAM size to set for the target
    function(pico_override_psram_size TARGET PSRAM_SIZE)
        set(PICO_PSRAM_SIZE_BYTES_STRING "${PSRAM_SIZE}")
        configure_file(${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_psram_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/pico_psram_region.ld)
        pico_add_linker_script_override_path(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} FILES pico_psram_region.ld)
        target_compile_definitions(${TARGET} PRIVATE "PICO_PSRAM_SIZE_BYTES=${PSRAM_SIZE}")
    endfunction()

    # pico_set_binary_type(TARGET TYPE)
    # \brief\ Set the binary type for the target
    #
    # \param\ TYPE The binary type to set
    function(pico_set_binary_type TARGET TYPE)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE ${TYPE})
        pico_add_memmap_link_depends(${TARGET} ${TYPE})
    endfunction()

    # PICO_CMAKE_CONFIG: PICO_DEFAULT_BINARY_TYPE, The default binary type to use, type=string, default=default, group=build
    # slightly messy as we support both the preferred PICO_DEFAULT_BINARY_TYPE and the individual variables
    if (NOT PICO_DEFAULT_BINARY_TYPE)
        if (PICO_NO_FLASH)
            set(PICO_DEFAULT_BINARY_TYPE no_flash)
        elseif (PICO_USE_BLOCKED_RAM)
            set(PICO_DEFAULT_BINARY_TYPE blocked_ram)
        elseif (PICO_COPY_TO_RAM)
            set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)
        else()
            set(PICO_DEFAULT_BINARY_TYPE default)
        endif()
    else()
        # we must set the individual variables here, as they are used in generator expressions,
        # but also for our checks below
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL no_flash)
            set(PICO_NO_FLASH 1)
        endif()
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL blocked_ram)
            set(PICO_USE_BLOCKED_RAM 1)
        endif()
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL copy_to_ram)
            set(PICO_COPY_TO_RAM 1)
        endif()
    endif()
    if ((PICO_NO_FLASH AND PICO_USE_BLOCKED_RAM) OR
        (PICO_USE_BLOCKED_RAM AND PICO_COPY_TO_RAM) OR
        (PICO_COPY_TO_RAM AND PICO_NO_FLASH))
        message(FATAL_ERROR "Conflicting binary types specified amongst PICO_DEFAULT_BINARY_TYPE, PICO_NO_FLASH, PICO_USE_BLOCKED_RAM and PICO_COPY_TO_RAM")
    endif()

    # todo only needed if not using a custom linker script
    if (NOT PICO_LINKER_SCRIPT_PATH)
        set(PICO_LINKER_SCRIPT_PATH "THIS_IS_THE_UNSET_PICO_LINKER_SCRIPT_PATH")
    endif()
    # configure the flash size in pico_flash_region.ld
    if (NOT PICO_FLASH_SIZE_BYTES)
        if (PICO_DEFAULT_FLASH_SIZE_BYTES)
            set(PICO_FLASH_SIZE_BYTES ${PICO_DEFAULT_FLASH_SIZE_BYTES})
        else()
            set(PICO_FLASH_SIZE_BYTES "2 * 1024 * 1024")
        endif()
    endif()
    # since linker script can handle expressions; may as well leave it as one
    #math(EXPR PICO_FLASH_SIZE_BYTES_STRING "${PICO_FLASH_SIZE_BYTES}" OUTPUT_FORMAT HEXADECIMAL)
    set(PICO_FLASH_SIZE_BYTES_STRING "${PICO_FLASH_SIZE_BYTES}")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/pico_flash_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/pico_flash_region.ld)
    # add include path for linker scripts to find it
    list(APPEND PICO_LINKER_SCRIPT_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})
    pico_promote_common_scope_vars()

    # add PSRAM region if it is enabled
    if (PICO_PSRAM_SIZE_BYTES)
        set(PICO_PSRAM_SIZE_BYTES_STRING "${PICO_PSRAM_SIZE_BYTES}")
    else()
        set(PICO_PSRAM_SIZE_BYTES_STRING "0")
    endif()
    configure_file(${CMAKE_CURRENT_LIST_DIR}/pico_psram_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/pico_psram_region.ld)

    # add override paths from PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS
    target_link_options(pico_standard_link INTERFACE "LINKER:$<$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS>>:-L$<JOIN:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS>,,-L>>")

    # add include paths from PICO_LINKER_SCRIPT_INCLUDE_DIRS
    foreach(DIR IN LISTS PICO_LINKER_SCRIPT_INCLUDE_DIRS)
        target_link_options(pico_standard_link INTERFACE "LINKER:-L${DIR}")
    endforeach()

    # add default locations script, so they can be referenced by pico_set_linker_script_var variables
    target_link_options(pico_standard_link INTERFACE "LINKER:--script=${PICO_LINKER_DEFAULT_LOCATIONS_PATH}")

    # add variables set by pico_set_linker_script_var function
    target_link_options(pico_standard_link INTERFACE "LINKER:$<$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_VARS>>:--defsym=$<JOIN:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_VARS>,,--defsym=>>")

    # LINKER script will be PICO_TARGET_LINKER_SCRIPT if set on target, or ${CMAKE_CURRENT_LIST_DIR}/memmap_foo.ld
    # if PICO_TARGET_BINARY_TYPE is set to foo on the target, otherwise ${CMAKE_CURRENT_LIST_DIR}/memmap_${PICO_DEFAULT_BINARY_TYPE).ld
    set(_LINKER_SCRIPT_EXPRESSION "$<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>>,$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>,${PICO_LINKER_SCRIPT_PATH}/memmap_$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,>,${PICO_DEFAULT_BINARY_TYPE},$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>.ld>")
    target_link_options(pico_standard_link INTERFACE
        # use -T instead of --script here, as picolibc.specs looks for any -T not --linker to suppress injection of its own linker script
        "-T${_LINKER_SCRIPT_EXPRESSION}"
            )
    pico_add_link_depend(pico_standard_link ${_LINKER_SCRIPT_EXPRESSION})
    unset(_LINKER_SCRIPT_EXPRESSION)

    pico_add_memmap_link_depends(pico_standard_link ${PICO_DEFAULT_BINARY_TYPE})

    # PICO_NO_FLASH will be set based on PICO_TARGET_BUILD_TYPE target property being equal to no_flash if set, otherwise to the value of the PICO_NO_FLASH cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_NO_FLASH, whether this is a 'no_flash' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_NO_FLASH=$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,no_flash>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)
    # PICO_USE_BLOCKED_RAM will be set based on PICO_TARGET_BUILD_TYPE target property being equal to use_blocked_ram if set, otherwise to the value of the PICO_USE_BLOCKED_RAM cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_USE_BLOCKED_RAM, whether this is a 'blocked_ram' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_USE_BLOCKED_RAM=$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,use_blocked_ram>,1,$<AND:$<BOOL:${PICO_USE_BLOCKED_RAM}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)
    # PICO_COPY_TO_RAM will be set based on PICO_TARGET_BUILD_TYPE target property being equal to copy_to_ram if set, otherwise to the value of the PICO_COPY_TO_RAM cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_COPY_TO_RAM, whether this is a 'copy_to_ram' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_COPY_TO_RAM=$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,copy_to_ram>,1,$<AND:$<BOOL:${PICO_COPY_TO_RAM}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)

    target_compile_definitions(pico_standard_link INTERFACE PICO_CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
    if (PICO_DEOPTIMIZED_DEBUG AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_definitions(pico_standard_link INTERFACE PICO_DEOPTIMIZED_DEBUG=1)
    endif()

    # this (arguably wrong) code is restored for 1.5.1 as setting -nostartfiles on many C++ binaries causes link errors. see issue #1368
    # -nostartfiles will be added if PICO_NO_FLASH would be defined to 1
    target_link_options(pico_standard_link INTERFACE $<$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,no_flash>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>:-nostartfiles>)
    # boot_stage2 will be linked if PICO_NO_FLASH would be defined to 0; note if boot_stage2 headers not present, then boot_stage2 is omitted from build anyway
    if (TARGET boot_stage2_headers)
        target_link_libraries(pico_standard_link INTERFACE $<$<NOT:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,no_flash>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>>:$<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_BOOT_STAGE2>>,$<TARGET_PROPERTY:PICO_TARGET_BOOT_STAGE2>,bs2_default>_library>)
    endif()

    # PICO_CMAKE_CONFIG: PICO_USE_DEFAULT_MAX_PAGE_SIZE, Don't shrink linker max page to 4096, type=bool, default=0, advanced=true, group=pico_standard_link
    if (NOT PICO_USE_DEFAULT_MAX_PAGE_SIZE)
        target_link_options(pico_standard_link INTERFACE "LINKER:-z,max-page-size=4096")
    endif()
    # done in compiler now
    #target_link_options(pico_standard_link INTERFACE "LINKER:--build-id=none")

    # this line occasionally useful for debugging ... todo maybe make a PICO_ var
    # target_compile_options(pico_standard_link INTERFACE --save-temps) #debugging only

    # PICO_CMAKE_CONFIG: PICO_NO_GC_SECTIONS, Disable `-ffunction-sections` `-fdata-sections` and `--gc-sections`, type=bool, default=0, advanced=true, group=pico_standard_link
    if (NOT PICO_NO_GC_SECTIONS)
        target_compile_options(pico_standard_link INTERFACE -ffunction-sections -fdata-sections)
        target_link_options(pico_standard_link INTERFACE "LINKER:--gc-sections")
    endif()

    if (PICO_C_COMPILER_IS_GNU)
        # Ignore warnings about rwx segments introduced in binutils 2.39
        execute_process(COMMAND ${CMAKE_C_COMPILER} -print-prog-name=ld RESULT_VARIABLE RUN_C_RESULT OUTPUT_VARIABLE FULL_LD_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (${RUN_C_RESULT} EQUAL 0)
            execute_process(COMMAND ${FULL_LD_PATH} --help RESULT_VARIABLE RUN_LD_RESULT OUTPUT_VARIABLE LD_HELP_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (${RUN_LD_RESULT} EQUAL 0)
                    set(RWX_WARNING "no-warn-rwx-segments")
                    string(FIND "${LD_HELP_OUTPUT}" "${RWX_WARNING}" LD_RWX_WARNING_SUPPORTED)
                    if (${LD_RWX_WARNING_SUPPORTED} GREATER -1)
                        target_link_options(pico_standard_link INTERFACE "LINKER:--${RWX_WARNING}")
                    endif()
            endif()
        endif()
    endif()
endif()
