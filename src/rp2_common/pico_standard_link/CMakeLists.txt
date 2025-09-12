if (NOT TARGET pico_standard_link)
    pico_add_library(pico_standard_link)

    if (TARGET boot_stage2_headers)
        target_link_libraries(pico_standard_link INTERFACE boot_stage2_headers)
    endif()

    # pico_read_addressmap_value(VARS ...)
    # \brief_nodesc\ Read the addressmap.h file and set the values as properties on pico_standard_link
    #
    # This function is used to read the addressmap.h file and set the values as properties on pico_standard_link,
    # for fetching later using pico_get_addressmap_value.
    #
    # \param\ VARS The variables to read the values of
    function(pico_read_addressmap_value)
        if (TARGET hardware_regs_headers)
            # parse addressmap.h for values, and set them as properties on pico_standard_link
            get_target_property(hardware_regs_headers_source_dir hardware_regs_headers SOURCE_DIR)
            if (NOT EXISTS ${hardware_regs_headers_source_dir}/include/hardware/regs/addressmap.h)
                message(STATUS "addressmap.h not found in ${hardware_regs_headers_source_dir}/include/hardware/regs/")
            else()
                file(READ ${hardware_regs_headers_source_dir}/include/hardware/regs/addressmap.h hardware_regs_addressmap_h)
                foreach (VAR ${ARGV})
                    string(REGEX MATCH "#define ${VAR} _u\\(([^)]*)\\)" TMP_VAR "${hardware_regs_addressmap_h}")
                    if (${CMAKE_MATCH_1})
                        set_target_properties(pico_standard_link PROPERTIES ${VAR} ${CMAKE_MATCH_1})
                    endif()
                endforeach()
            endif()
        endif()
    endfunction()

    # Read all the standard variables from the addressmap.h file
    pico_read_addressmap_value(
        XIP_SRAM_BASE XIP_SRAM_END
        SRAM_BASE SRAM_STRIPED_END
        SRAM4_BASE SRAM5_BASE
        SRAM_SCRATCH_X_BASE SRAM_SCRATCH_Y_BASE
        SRAM_END
    )

    # pico_get_addressmap_value(VAR [FALLBACK_VAR <VAR>])
    # \brief\ Get the value of a variable from the addressmap.h file
    #
    # Use pico_read_addressmap_value to read all the values you need from the addressmap.h 
    # file before using this function, for best performance.
    #
    # This function will use the fallback variable if the first variable is not found.
    #
    # If neither are found, it will call pico_read_addressmap_value to read the values
    # from the addressmap.h file, and throw an error if the values are still not found.
    #
    # \param\ VAR The variable to get the value of
    # \param\ FALLBACK_VAR The variable to use if VAR is not found
    function(pico_get_addressmap_value VAR)
        set(zeroValueArgs NO_RECURSE)
        set(oneValueArgs FALLBACK_VAR)
        cmake_parse_arguments(PARSE_ARGV 0 args
            "${zeroValueArgs}" "${oneValueArgs}" ""
        )

        get_target_property(TMP_VAR pico_standard_link ${VAR})
        if (TMP_VAR)
            set(${VAR} ${TMP_VAR} PARENT_SCOPE)
            return()
        elseif (args_FALLBACK_VAR)
            get_target_property(TMP_VAR pico_standard_link ${args_FALLBACK_VAR})
            if (TMP_VAR)
                set(${VAR} ${TMP_VAR} PARENT_SCOPE)
                return()
            endif()
        endif()

        # Attempt to read the value from the addressmap.h file
        if (NOT args_NO_RECURSE)
            pico_read_addressmap_value(${VAR} ${args_FALLBACK_VAR})
            pico_get_addressmap_value(${VAR} NO_RECURSE FALLBACK_VAR ${args_FALLBACK_VAR})
            set(${VAR} ${${VAR}} PARENT_SCOPE)
            return()
        else()
            message(FATAL_ERROR "Variable ${VAR} and fallback ${args_FALLBACK_VAR} not found in addressmap.h")
        endif()
    endfunction()

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

    # pico_set_binary_type(TARGET TYPE)
    # \brief\ Set the binary type for the target
    #
    # \param\ TYPE The binary type to set
    function(pico_set_binary_type TARGET TYPE)
        # catch special cases
        if (TYPE STREQUAL "xip_sram")
            pico_get_addressmap_value(XIP_SRAM_BASE)
            pico_get_addressmap_value(XIP_SRAM_END)
            target_compile_definitions(${TARGET} PRIVATE PICO_USE_XIP_CACHE_AS_RAM=1)
            set(TYPE "sram")
            if (PICO_RP2040)
                # Use copy_flash_sram, as RP2040 bootrom cannot directly access XIP SRAM
                set(TYPE "copy_flash_sram")
            elseif (PICO_RP2350)
                # Exclusives only work in main SRAM on RP2350, so cannot use software spin locks
                target_compile_definitions(${TARGET} PRIVATE PICO_USE_XIP_CACHE_AS_RAM=1 PICO_USE_SW_SPIN_LOCKS=0)
            endif()
            pico_set_modified_binary_type(${TARGET} ${TYPE} RAM ${XIP_SRAM_BASE} ${XIP_SRAM_END}-${XIP_SRAM_BASE}-4k SCRATCH_X ${XIP_SRAM_END}-4k 2k SCRATCH_Y ${XIP_SRAM_END}-2k 2k)
            return()
        elseif (TYPE STREQUAL "no_flash_using_xip_ram" OR TYPE STREQUAL "copy_to_ram_using_xip_ram")
            function(pico_internal_add_xip_text_section)
                set(ADDITIONAL_PRE_DATA "set(ADDITIONAL_PRE_DATA \"
                .xip_text : {
                    __xip_ram_text_start__ = .;
                    . = ALIGN(4);
                    *(.time_critical.text*)
                    . = ALIGN(4);
                    __xip_ram_text_end__ = .;
                } > XIP_RAM AT> FLASH
                __xip_ram_text_source__ = LOADADDR(.xip_text);\")" PARENT_SCOPE)

                pico_get_addressmap_value(XIP_SRAM_BASE)
                pico_get_addressmap_value(XIP_SRAM_END)
                set(ADDITIONAL_MEMORY "set(ADDITIONAL_MEMORY \"    XIP_RAM(rwx) : ORIGIN = ${XIP_SRAM_BASE}, LENGTH = ${XIP_SRAM_END} - ${XIP_SRAM_BASE}\")" PARENT_SCOPE)
            endfunction()
            target_compile_definitions(${TARGET} PRIVATE PICO_DATA_COPY_EXTRA_SECTIONS_FILE="data_cpy_xip_text.S" PICO_USE_XIP_CACHE_AS_RAM=1)
            file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/data_cpy_xip_text.S "
.word __xip_ram_text_source__
.word __xip_ram_text_start__
.word __xip_ram_text_end__
")
            target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
            if (TYPE STREQUAL "no_flash_using_xip_ram")
                pico_set_modified_binary_type(${TARGET} sram EXTRAS pico_internal_add_xip_text_section)
            elseif (TYPE STREQUAL "copy_to_ram_using_xip_ram")
                pico_set_modified_binary_type(${TARGET} copy_flash_sram EXTRAS pico_internal_add_xip_text_section)
            endif()
            return()
        endif()
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE ${TYPE})
    endfunction()

    # pico_set_modified_binary_type(TARGET TYPE [RAM <RAM_ORIGIN> <RAM_LENGTH>] [SCRATCH_X <SCRATCH_X_ORIGIN> <SCRATCH_X_LENGTH>] [SCRATCH_Y <SCRATCH_Y_ORIGIN> <SCRATCH_Y_LENGTH>] [EXTRAS <EXTRA_FN>])
    # \brief\ Set the binary type for the target with modified regions
    #
    # \param\ TYPE The type of binary to use (flash, sram, or copy_flash_sram)
    # \param\ RAM_ORIGIN The origin of the RAM section
    # \param\ RAM_LENGTH The length of the RAM section
    # \param\ SCRATCH_X_ORIGIN The origin of the SCRATCH_X section
    # \param\ SCRATCH_X_LENGTH The length of the SCRATCH_X section
    # \param\ SCRATCH_Y_ORIGIN The origin of the SCRATCH_Y section
    # \param\ SCRATCH_Y_LENGTH The length of the SCRATCH_Y section
    # \param\ EXTRAS A CMake function to call to set additional variables - requires CMake 3.18.0 or later
    function(pico_set_modified_binary_type TARGET TYPE)
        set(oneValueArgs EXTRAS)
        set(multiValueArgs RAM SCRATCH_X SCRATCH_Y)
        cmake_parse_arguments(PARSE_ARGV 0 args
            "" "${oneValueArgs}" "${multiValueArgs}"
        )

        if (NOT (TYPE STREQUAL "sram" OR TYPE STREQUAL "flash" OR TYPE STREQUAL "copy_flash_sram"))
            message(FATAL_ERROR "Invalid binary type for pico_set_modified_binary_type: ${TYPE}")
        endif()

        # Default RAM, SCRATCH_X and SCRATCH_Y regions
        pico_get_addressmap_value(SRAM_BASE)
        pico_get_addressmap_value(SRAM_STRIPED_END)
        pico_get_addressmap_value(SRAM_SCRATCH_X_BASE FALLBACK_VAR SRAM4_BASE)  # fallback for RP2040
        pico_get_addressmap_value(SRAM_SCRATCH_Y_BASE FALLBACK_VAR SRAM5_BASE)  # fallback for RP2040
        pico_get_addressmap_value(SRAM_END)

        # Convert sizes to kibibytes - not necessary, but makes the scripts easier to read
        set(PICO_DEFAULT_RAM_ORIGIN ${SRAM_BASE})
        math(EXPR PICO_DEFAULT_RAM_LENGTH "(${SRAM_STRIPED_END}-${SRAM_BASE})/1024")
        set(PICO_DEFAULT_RAM_LENGTH "${PICO_DEFAULT_RAM_LENGTH}k")
        set(PICO_DEFAULT_SCRATCH_X_ORIGIN ${SRAM_SCRATCH_X_BASE})
        math(EXPR PICO_DEFAULT_SCRATCH_X_LENGTH "(${SRAM_SCRATCH_Y_BASE}-${SRAM_SCRATCH_X_BASE})/1024")
        set(PICO_DEFAULT_SCRATCH_X_LENGTH "${PICO_DEFAULT_SCRATCH_X_LENGTH}k")
        set(PICO_DEFAULT_SCRATCH_Y_ORIGIN ${SRAM_SCRATCH_Y_BASE})
        math(EXPR PICO_DEFAULT_SCRATCH_Y_LENGTH "(${SRAM_END}-${SRAM_SCRATCH_Y_BASE})/1024")
        set(PICO_DEFAULT_SCRATCH_Y_LENGTH "${PICO_DEFAULT_SCRATCH_Y_LENGTH}k")

        # Scripts that will be created by this function
        set(LINKER_CMAKE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/memmap_${TARGET}.cmake")
        set(LINKER_LD_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/memmap_${TARGET}.ld")

        # Configure memmap_${TARGET}.cmake script file to create memmap_${TARGET}.ld
        if (args_RAM)
            list(POP_FRONT args_RAM ORIGIN LENGTH)
            set(RAM "set(RAM_ORIGIN ${ORIGIN})\nset(RAM_LENGTH ${LENGTH})\n")
        elseif (PICO_DEFAULT_RAM_ORIGIN)
            set(RAM "set(RAM_ORIGIN ${PICO_DEFAULT_RAM_ORIGIN})\nset(RAM_LENGTH ${PICO_DEFAULT_RAM_LENGTH})\n")
        else()
            message(FATAL_ERROR "Unknown default RAM region for ${PICO_PLATFORM}")
        endif()
        if (args_SCRATCH_X)
            list(POP_FRONT args_SCRATCH_X ORIGIN LENGTH)
            set(SCRATCH_X "set(SCRATCH_X_ORIGIN ${ORIGIN})\nset(SCRATCH_X_LENGTH ${LENGTH})\n")
        elseif(PICO_DEFAULT_SCRATCH_X_ORIGIN)
            set(SCRATCH_X "set(SCRATCH_X_ORIGIN ${PICO_DEFAULT_SCRATCH_X_ORIGIN})\nset(SCRATCH_X_LENGTH ${PICO_DEFAULT_SCRATCH_X_LENGTH})\n")
        else()
            message(FATAL_ERROR "Unknown default SCRATCH_X region for ${PICO_PLATFORM}")
        endif()
        if (args_SCRATCH_Y)
            list(POP_FRONT args_SCRATCH_Y ORIGIN LENGTH)
            set(SCRATCH_Y "set(SCRATCH_Y_ORIGIN ${ORIGIN})\nset(SCRATCH_Y_LENGTH ${LENGTH})\n")
        elseif(PICO_DEFAULT_SCRATCH_Y_ORIGIN)
            set(SCRATCH_Y "set(SCRATCH_Y_ORIGIN ${PICO_DEFAULT_SCRATCH_Y_ORIGIN})\nset(SCRATCH_Y_LENGTH ${PICO_DEFAULT_SCRATCH_Y_LENGTH})\n")
        else()
            message(FATAL_ERROR "Unknown default SCRATCH_Y region for ${PICO_PLATFORM}")
        endif()

        if (args_EXTRAS)
            if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
                cmake_language(CALL ${args_EXTRAS})
            else()
                message(FATAL_ERROR "EXTRAS argument requires CMake 3.18.0 or later")
            endif()
        endif()

        if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17.0")
            # CMAKE_CURRENT_FUNCTION_LIST_DIR added in 3.17.0
            configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/memmap_script.template.cmake ${LINKER_CMAKE_SCRIPT} @ONLY)
        else()
            configure_file(${PICO_SDK_PATH}src/rp2_common/pico_standard_link/memmap_script.template.cmake ${LINKER_CMAKE_SCRIPT} @ONLY)
        endif()

        # Add command to run this script whenever it changes
        add_custom_command(OUTPUT ${LINKER_LD_SCRIPT}
            COMMAND ${CMAKE_COMMAND}
                -DPICO_LINKER_SCRIPT_PATH:PATH=${PICO_LINKER_SCRIPT_PATH}
                -Doutput_file:FILEPATH=${LINKER_LD_SCRIPT}
                -P "${LINKER_CMAKE_SCRIPT}"
            DEPENDS ${LINKER_CMAKE_SCRIPT} ${PICO_LINKER_SCRIPT_PATH}/memmap_${TYPE}.template.ld)
        add_custom_target(memmap_${TARGET}_ld DEPENDS ${LINKER_LD_SCRIPT})
        add_dependencies(${TARGET} memmap_${TARGET}_ld)

        pico_set_linker_script(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/memmap_${TARGET}.ld)

        # Set PICO_TARGET_BINARY_TYPE for additional configuration
        if (TYPE STREQUAL "sram")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE no_flash)
        elseif (TYPE STREQUAL "copy_flash_sram")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE copy_to_ram)
        else()
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE default)
        endif()
    endfunction()

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
    configure_file(${CMAKE_CURRENT_LIST_DIR}/pico_flash_region.template.ld ${CMAKE_BINARY_DIR}/pico_flash_region.ld)
    # add include path for linker scripts
    target_link_options(pico_standard_link INTERFACE "LINKER:-L${CMAKE_BINARY_DIR}")

    # LINKER script will be PICO_TARGET_LINKER_SCRIPT if set on target, or ${CMAKE_CURRENT_LIST_DIR}/memmap_foo.ld
    # if PICO_TARGET_BINARY_TYPE is set to foo on the target, otherwise ${CMAKE_CURRENT_LIST_DIR}/memmap_${PICO_DEFAULT_BINARY_TYPE).ld
    set(_LINKER_SCRIPT_EXPRESSION "$<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>>,$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>,${PICO_LINKER_SCRIPT_PATH}/memmap_$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,>,${PICO_DEFAULT_BINARY_TYPE},$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>.ld>")
    target_link_options(pico_standard_link INTERFACE
        "LINKER:--script=${_LINKER_SCRIPT_EXPRESSION}"
            )
    pico_add_link_depend(pico_standard_link ${_LINKER_SCRIPT_EXPRESSION})
    unset(_LINKER_SCRIPT_EXPRESSION)

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
