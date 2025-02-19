# Picotool Property Defines
# All INHERITED, so can be defined globally, or per target
#
# The picotool functions all set the specified target properties,
# and therefore if the property should be set for multiple targets
# then it can be set manually with `set_property` or other CMake
# functions to set properties for a given scope.
define_property(TARGET
    PROPERTY PICOTOOL_OTP_FILE
    INHERITED
    BRIEF_DOCS "OTP File to write"
    FULL_DOCS "OTP File to write"
)
define_property(TARGET
    PROPERTY PICOTOOL_SIGN_OUTPUT
    INHERITED
    BRIEF_DOCS "Sign output"
    FULL_DOCS "Sign output"
)
define_property(TARGET
    PROPERTY PICOTOOL_SIGFILE
    INHERITED
    BRIEF_DOCS "Private key for signing"
    FULL_DOCS "Private key for signing"
)
define_property(TARGET
    PROPERTY PICOTOOL_HASH_OUTPUT
    INHERITED
    BRIEF_DOCS "Hash output"
    FULL_DOCS "Hash output"
)
define_property(TARGET
    PROPERTY PICOTOOL_EMBED_PT
    INHERITED
    BRIEF_DOCS "Partition table to embed in output"
    FULL_DOCS "Partition table to embed in output"
)
define_property(TARGET
    PROPERTY PICOTOOL_AESFILE
    INHERITED
    BRIEF_DOCS "AES key for encrypting"
    FULL_DOCS "AES key for encrypting"
)
define_property(TARGET
    PROPERTY PICOTOOL_ENC_SIGFILE
    INHERITED
    BRIEF_DOCS "Private key for signing encrypted binaries"
    FULL_DOCS "Private key for signing encrypted binaries"
)
define_property(TARGET
    PROPERTY PICOTOOL_UF2_PACKAGE_ADDR
    INHERITED
    BRIEF_DOCS "Address to package UF2 at"
    FULL_DOCS "Address to package UF2 at"
)
define_property(TARGET
    PROPERTY PICOTOOL_UF2_FAMILY
    INHERITED
    BRIEF_DOCS "UF2 family to use"
    FULL_DOCS "UF2 family to use"
)
define_property(TARGET
    PROPERTY PICOTOOL_EXTRA_PROCESS_ARGS
    INHERITED
    BRIEF_DOCS "Extra arguments to pass to processing"
    FULL_DOCS "Extra arguments to pass to processing"
)
define_property(TARGET
    PROPERTY PICOTOOL_EXTRA_UF2_ARGS
    INHERITED
    BRIEF_DOCS "Extra arguments to pass to uf2 conversion"
    FULL_DOCS "Extra arguments to pass to uf2 conversion"
)

# Check pioasm is installed, or build it if not installed
function(pico_init_pioasm)
    # Assemble the version string from components instead of using PICO_SDK_VERSION_STRING, because the version string
    # potentially has a PRE_RELEASE_ID suffix, which will trip up the find_package call.
    set(pioasm_VERSION_REQUIRED "${PICO_SDK_VERSION_MAJOR}.${PICO_SDK_VERSION_MINOR}.${PICO_SDK_VERSION_REVISION}")
    if (NOT TARGET pioasm AND NOT DEFINED pioasm_FOUND)
        set(pioasm_INSTALL_DIR ${CMAKE_BINARY_DIR}/pioasm-install)
        if (NOT pioasm_DIR AND EXISTS ${pioasm_INSTALL_DIR}/pioasm)
            set(pioasm_DIR ${pioasm_INSTALL_DIR}/pioasm)
        endif()
        # Find package - will find installed pioasm, either at pioasm_DIR or system
        find_package(pioasm ${pioasm_VERSION_REQUIRED} QUIET CONFIG NO_CMAKE_FIND_ROOT_PATH)

        if (NOT pioasm_FOUND)
            set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PICO_SDK_PATH}/tools)
            # todo CMAKE_CURRENT_FUNCTION_LIST_DIR ... what version?
            find_package(pioasm MODULE REQUIRED)
        endif()
    endif()

    if (TARGET pioasm)
        set(pioasm_FOUND 1 PARENT_SCOPE)
    else()
        message("No pioasm found")
    endif()
endfunction()

# Check picotool is installed, or download and build it if not installed
function(pico_init_picotool)
    set(picotool_VERSION_REQUIRED 2.1.1)
    if (NOT TARGET picotool AND NOT DEFINED picotool_FOUND)
        # Build path of local install dir
        if (DEFINED ENV{PICOTOOL_FETCH_FROM_GIT_PATH} AND (NOT PICOTOOL_FETCH_FROM_GIT_PATH))
            set(PICOTOOL_FETCH_FROM_GIT_PATH $ENV{PICOTOOL_FETCH_FROM_GIT_PATH})
            message("Using PICOTOOL_FETCH_FROM_GIT_PATH from environment ('${PICOTOOL_FETCH_FROM_GIT_PATH}')")
        endif ()
        include(FetchContent)
        if (PICOTOOL_FETCH_FROM_GIT_PATH)
            get_filename_component(picotool_INSTALL_DIR "${PICOTOOL_FETCH_FROM_GIT_PATH}" ABSOLUTE)
        else()
            set(picotool_INSTALL_DIR ${FETCHCONTENT_BASE_DIR})
        endif ()

        if (NOT PICOTOOL_FORCE_FETCH_FROM_GIT AND NOT ENV{PICOTOOL_FORCE_FETCH_FROM_GIT})
            if (NOT picotool_DIR AND EXISTS ${picotool_INSTALL_DIR}/picotool)
                set(picotool_DIR ${picotool_INSTALL_DIR}/picotool)
            endif()
            # Find package - will find installed picotool, either at picotool_DIR or system
            find_package(picotool ${picotool_VERSION_REQUIRED} QUIET CONFIG NO_CMAKE_FIND_ROOT_PATH)
            if (NOT picotool_FOUND AND picotool_CONSIDERED_VERSIONS)
                message(FATAL_ERROR
                    "Incompatible picotool installation found: "
                    "Requires version ${picotool_VERSION_REQUIRED}, "
                    "you have version ${picotool_CONSIDERED_VERSIONS}\n"
                    "Update your installation, or set PICOTOOL_FORCE_FETCH_FROM_GIT "
                    "to download and build the correct version"
                )
            endif()
        else()
            message("Forcing fetch of picotool from git")
        endif()
        if (NOT picotool_FOUND)
            set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PICO_SDK_PATH}/tools)
            find_package(picotool MODULE)
        endif()
    endif()

    if (TARGET picotool)
        set(picotool_FOUND 1 PARENT_SCOPE)
    else()
        message("No picotool found")
    endif()
endfunction()

function(picotool_check_configurable TARGET)
    get_target_property(configured ${TARGET} PICOTOOL_PROCESSING_CONFIGURED)
    if (configured)
        message(FATAL_ERROR "All picotool post-processing functions for \"${TARGET}\" must come before pico_add_extra_outputs(${TARGET})")
    endif()
endfunction()

# Generate pio header and include it in the build
# PICO_CMAKE_CONFIG: PICO_DEFAULT_PIOASM_OUTPUT_FORMAT, Default output format used by pioasm when using pico_generate_pio_header, type=string, default=c-sdk, group=build
function(pico_generate_pio_header TARGET)
    pico_init_pioasm()
    # Note that PATH is not a valid argument but was previously ignored (and happens to be passed by pico-extras)
    cmake_parse_arguments(pico_generate_pio_header "" "OUTPUT_FORMAT;OUTPUT_DIR;PATH" "" ${ARGN} )

    if (pico_generate_pio_header_OUTPUT_FORMAT)
        set(OUTPUT_FORMAT "${pico_generate_pio_header_OUTPUT_FORMAT}")
    elseif(DEFINED PICO_DEFAULT_PIOASM_OUTPUT_FORMAT)
        set(OUTPUT_FORMAT "${PICO_DEFAULT_PIOASM_OUTPUT_FORMAT}")
    else()
        set(OUTPUT_FORMAT "c-sdk")
    endif()

    if (pico_generate_pio_header_OUTPUT_DIR)
        file(MAKE_DIRECTORY ${pico_generate_pio_header_OUTPUT_DIR})
        get_filename_component(HEADER_DIR ${pico_generate_pio_header_OUTPUT_DIR} ABSOLUTE)
    else()
        set(HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # Loop through each PIO file
    foreach(PIO ${pico_generate_pio_header_UNPARSED_ARGUMENTS})
        get_filename_component(PIO_NAME ${PIO} NAME)
        set(HEADER "${HEADER_DIR}/${PIO_NAME}.h")
        #message("Will generate ${HEADER}")
        get_filename_component(HEADER_GEN_TARGET ${PIO} NAME_WE)
        set(HEADER_GEN_TARGET "${TARGET}_${HEADER_GEN_TARGET}_pio_h")

        add_custom_target(${HEADER_GEN_TARGET} DEPENDS ${HEADER})

        if (PICO_PIO_VERSION)
            set(VERSION_STRING "${PICO_PIO_VERSION}")
        else()
            set(VERSION_STRING "0")
        endif()

        add_custom_command(OUTPUT ${HEADER}
                DEPENDS ${PIO}
                COMMAND pioasm -o ${OUTPUT_FORMAT} -v ${VERSION_STRING} ${PIO} ${HEADER}
                VERBATIM)

        add_dependencies(${TARGET} ${HEADER_GEN_TARGET})
    endforeach()

    get_target_property(target_type ${TARGET} TYPE)
    if ("INTERFACE_LIBRARY" STREQUAL "${target_type}")
        target_include_directories(${TARGET} INTERFACE ${HEADER_DIR})
    else()
        target_include_directories(${TARGET} PUBLIC ${HEADER_DIR})
    endif()
endfunction()

# pico_package_uf2_output(TARGET PACKADDR)
# Package a UF2 output to be written to the PACKADDR address. This can be
# used with a no_flash binary to write the UF2 to flash when dragging &
# dropping, and it will be copied to SRAM by the bootrom before execution.
# This sets PICOTOOL_UF2_PACKAGE_ADDR to PACKADDR.
function(pico_package_uf2_output TARGET PACKADDR)
    picotool_check_configurable(${TARGET})
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_UF2_PACKAGE_ADDR ${PACKADDR}
    )
endfunction()

# pico_set_otp_key_output_file(TARGET OTPFILE)
# Output the public key hash and other necessary rows to an otp JSON file.
# This sets PICOTOOL_OTP_FILE to OTPFILE.
function(pico_set_otp_key_output_file TARGET OTPFILE)
    picotool_check_configurable(${TARGET})
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_OTP_FILE ${OTPFILE}
    )
endfunction()

# pico_load_map_clear_sram(TARGET)
# Adds an entry to the load map to instruct the bootrom to clear all of SRAM
# before loading the binary. This appends the `--clear` argument
# to PICOTOOL_EXTRA_PROCESS_ARGS.
function(pico_load_map_clear_sram TARGET)
    picotool_check_configurable(${TARGET})
    # get and set, to inherit list
    get_target_property(extra_args ${TARGET} PICOTOOL_EXTRA_PROCESS_ARGS)
    if (extra_args)
        set_target_properties(${TARGET} PROPERTIES PICOTOOL_EXTRA_PROCESS_ARGS ${extra_args})
    endif()
    # append --clear to list
    set_property(TARGET ${TARGET} APPEND PROPERTY
        PICOTOOL_EXTRA_PROCESS_ARGS "--clear"
    )
endfunction()

# pico_set_binary_version(<TARGET> [MAJOR <version>] [MINOR <version>] [ROLLBACK <version>] [ROLLBACK_ROWS <rows...>])
# Adds a version item to the metadata block, with the given major, minor and
# rollback version, along with the rollback rows. These are appended as arguments
# to PICOTOOL_EXTRA_PROCESS_ARGS if setting the rollback version, or set as compile
# definitions if only setting the major/minor versions.
function(pico_set_binary_version TARGET)
    picotool_check_configurable(${TARGET})
    set(oneValueArgs MAJOR MINOR ROLLBACK)
    set(multiValueArgs ROWS)
    cmake_parse_arguments(PARSE_ARGV 1 SV "" "${oneValueArgs}" "${multiValueArgs}")
    # get and set, to inherit list
    get_target_property(extra_args ${TARGET} PICOTOOL_EXTRA_PROCESS_ARGS)
    if (extra_args)
        set_target_properties(${TARGET} PROPERTIES PICOTOOL_EXTRA_PROCESS_ARGS ${extra_args})
    endif()
    if (SV_ROLLBACK)
        if (SV_MAJOR)
            # append major version
            set_property(TARGET ${TARGET} APPEND PROPERTY
                PICOTOOL_EXTRA_PROCESS_ARGS "--major" "${SV_MAJOR}"
            )
        endif()
        if (SV_MINOR)
            # append minor version
            set_property(TARGET ${TARGET} APPEND PROPERTY
                PICOTOOL_EXTRA_PROCESS_ARGS "--minor" "${SV_MINOR}"
            )
        endif()
        # append rollback version
        set_property(TARGET ${TARGET} APPEND PROPERTY
            PICOTOOL_EXTRA_PROCESS_ARGS "--rollback" "${SV_ROLLBACK}"
        )
        if (SV_ROWS)
            # append rollback rows
            foreach(row IN LISTS SV_ROWS)
                set_property(TARGET ${TARGET} APPEND PROPERTY
                    PICOTOOL_EXTRA_PROCESS_ARGS "${row}"
                )
            endforeach()
        endif()
    else()
        if (SV_MAJOR)
            # set major version
            target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_VERSION_MAJOR=${SV_MAJOR})
        endif()
        if (SV_MINOR)
            # append minor version
            target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_VERSION_MINOR=${SV_MINOR})
        endif()
    endif()
endfunction()

# pico_set_uf2_family(TARGET FAMILY)
# Set the UF2 family to use when creating the UF2.
# This sets PICOTOOL_UF2_FAMILY to FAMILY.
function(pico_set_uf2_family TARGET FAMILY)
    picotool_check_configurable(${TARGET})
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_UF2_FAMILY ${FAMILY}
    )
endfunction()

# pico_sign_binary(TARGET [SIGFILE])
# Sign the target binary with the given PEM signature. This sets
# PICOTOOL_SIGN_OUTPUT to true, PICOTOOL_SIGFILE to SIGFILE (if specified),
# and PICOTOOL_OTP_FILE to ${TARGET}.otp.json (if not already set). To
# specify a common SIGFILE for multiple targets, the SIGFILE property can be
# set for a given scope, and then the SIGFILE argument is optional.
function(pico_sign_binary TARGET)
    picotool_check_configurable(${TARGET})
    # Enforce signing through target properties
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_SIGN_OUTPUT true
    )
    if (ARGC EQUAL 2)
        set_target_properties(${TARGET} PROPERTIES
            PICOTOOL_SIGFILE ${ARGV1}
        )
    else()
        get_target_property(sig_file ${TARGET} PICOTOOL_SIGFILE)
        if (NOT sig_file)
            message(FATAL_ERROR "Signature file not set for ${TARGET}")
        endif()
    endif()
    get_target_property(otp_file ${TARGET} PICOTOOL_OTP_FILE)
    if (NOT otp_file)
        set_target_properties(${TARGET} PROPERTIES
            PICOTOOL_OTP_FILE "${TARGET}.otp.json"
        )
    endif()
endfunction()

# pico_hash_binary(TARGET)
# Hash the target binary. This sets PICOTOOL_HASH_OUTPUT to true.
function(pico_hash_binary TARGET)
    picotool_check_configurable(${TARGET})
    # Enforce hashing through target properties
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_HASH_OUTPUT true
    )
endfunction()

# pico_embed_pt_in_binary(TARGET PTFILE)
# Create the specified partition table from JSON, and embed it in the
# block loop. This sets PICOTOOL_EMBED_PT to PTFILE.
function(pico_embed_pt_in_binary TARGET PTFILE)
    picotool_check_configurable(${TARGET})
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_EMBED_PT ${PTFILE}
    )
endfunction()

# pico_encrypt_binary(TARGET AESFILE [SIGFILE])
# Encrypt the target binary with the given AES key (should be a binary
# file containing 32 bytes of a random key), and sign the encrypted binary.
# This sets PICOTOOL_AESFILE to AESFILE, and PICOTOOL_ENC_SIGFILE to SIGFILE
# if present, else PICOTOOL_SIGFILE.
function(pico_encrypt_binary TARGET AESFILE)
    picotool_check_configurable(${TARGET})
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_AESFILE ${AESFILE}
    )
    if (ARGC EQUAL 3)
        set_target_properties(${TARGET} PROPERTIES
            PICOTOOL_ENC_SIGFILE ${ARGV2}
        )
    else()
        get_target_property(enc_sig_file ${TARGET} PICOTOOL_ENC_SIGFILE)
        if (NOT enc_sig_file)
            get_target_property(sig_file ${TARGET} PICOTOOL_SIGFILE)
            if (NOT sig_file)
                message(FATAL_ERROR "Signature file not set for ${TARGET}")
            else()
                set_target_properties(${TARGET} PROPERTIES
                    PICOTOOL_ENC_SIGFILE ${sig_file}
                )
            endif()
        endif()
    endif()
endfunction()

# pico_add_uf2_output(TARGET)
# Add a UF2 output using picotool - must be called after
# all required properties have been set
function(pico_add_uf2_output TARGET)
    pico_init_picotool()
    get_target_property(${TARGET}_archive_directory ${TARGET} ARCHIVE_OUTPUT_DIRECTORY)
    if (${TARGET}_archive_directory)
        get_filename_component(output_path "${${TARGET}_archive_directory}"
                REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
        file(MAKE_DIRECTORY "${output_path}")
        set(output_path "${output_path}/")
    else()
        set(output_path "")
    endif()

    get_target_property(${TARGET}_uf2_package_addr ${TARGET} PICOTOOL_UF2_PACKAGE_ADDR)
    if (${TARGET}_uf2_package_addr)
        set(uf2_package_args "-o;${${TARGET}_uf2_package_addr}")
    endif()

    get_target_property(extra_uf2_args ${TARGET} PICOTOOL_EXTRA_UF2_ARGS)
    if (PICO_RP2350_A2_SUPPORTED)
        if (NOT extra_uf2_args)
            set(extra_uf2_args "--abs-block")
        elseif(NOT "--abs-block" IN_LIST extra_uf2_args)
            list(APPEND extra_uf2_args "--abs-block")
        endif()
    else()
        if (NOT extra_uf2_args)
            set(extra_uf2_args "")
        endif()
    endif()

    if (picotool_FOUND)
        get_target_property(picotool_family ${TARGET} PICOTOOL_UF2_FAMILY)
        if (NOT picotool_family)
            if (PICOTOOL_DEFAULT_FAMILY)
                set(picotool_family ${PICOTOOL_DEFAULT_FAMILY})
            else()
                set(picotool_family ${PICO_PLATFORM})
            endif()
        endif()
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND picotool
            ARGS uf2 convert
                --quiet
                ${uf2_package_args}
                $<TARGET_FILE:${TARGET}>
                ${output_path}$<IF:$<BOOL:$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>>,$<TARGET_PROPERTY:${TARGET},OUTPUT_NAME>,$<TARGET_PROPERTY:${TARGET},NAME>>.uf2
                --family ${picotool_family}
                ${extra_uf2_args}
            COMMAND_EXPAND_LISTS
            VERBATIM)
    endif()
endfunction()

# Run picotool post-processing on the binary - must be called after
# all required properties have been set
function(picotool_postprocess_binary TARGET)
    set_target_properties(${TARGET} PROPERTIES
        PICOTOOL_PROCESSING_CONFIGURED true
    )
    # Read target properties
    get_target_property(picotool_sign_output ${TARGET} PICOTOOL_SIGN_OUTPUT)
    if (picotool_sign_output)
        list(APPEND picotool_args "--sign")
        get_target_property(picotool_sigfile ${TARGET} PICOTOOL_SIGFILE)
        pico_add_link_depend(${TARGET} ${picotool_sigfile})
    endif()

    get_target_property(picotool_hash_output ${TARGET} PICOTOOL_HASH_OUTPUT)
    if (picotool_hash_output)
        list(APPEND picotool_args "--hash")
    endif()

    get_target_property(otp_file ${TARGET} PICOTOOL_OTP_FILE)
    if (NOT otp_file)
        set(otp_file "")
    endif()
    get_target_property(uf2_package_addr ${TARGET} PICOTOOL_UF2_PACKAGE_ADDR)

    # Embed PT properties
    get_target_property(picotool_embed_pt ${TARGET} PICOTOOL_EMBED_PT)
    if (picotool_embed_pt)
        pico_add_link_depend(${TARGET} ${picotool_embed_pt})
    endif()

    # Encryption properties
    get_target_property(picotool_aesfile ${TARGET} PICOTOOL_AESFILE)
    if (picotool_aesfile)
        pico_add_link_depend(${TARGET} ${picotool_aesfile})
    endif()
    get_target_property(picotool_enc_sigfile ${TARGET} PICOTOOL_ENC_SIGFILE)
    if (picotool_enc_sigfile)
        pico_add_link_depend(${TARGET} ${picotool_enc_sigfile})
    endif()

    # Extra args
    get_target_property(extra_process_args ${TARGET} PICOTOOL_EXTRA_PROCESS_ARGS)
    if (extra_process_args)
        list(APPEND picotool_args ${extra_process_args})
    endif()

    pico_init_picotool()
    if (picotool_FOUND)
        # Embed PT
        if (picotool_embed_pt)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                DEPENDS ${picotool_embed_pt}
                COMMAND picotool partition create --quiet ${picotool_embed_pt} $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>
                VERBATIM)
        endif()
        # Signing/hashing/load maps for packaging
        if (
            picotool_sign_output OR
            picotool_hash_output OR
            uf2_package_addr OR
            extra_process_args
        )
        add_custom_command(TARGET ${TARGET} POST_BUILD
            DEPENDS ${picotool_sigfile}
            COMMAND picotool
            ARGS seal
                --quiet
                $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>
                ${picotool_sigfile} ${otp_file}
                ${picotool_args}
            COMMAND_EXPAND_LISTS
            VERBATIM)
        endif()
        # Encryption
        if (picotool_aesfile)
            add_custom_command(TARGET ${TARGET} POST_BUILD
                DEPENDS ${picotool_enc_sigfile} ${picotool_aesfile}
                COMMAND picotool encrypt --quiet --hash --sign $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}> ${picotool_aesfile} ${picotool_enc_sigfile}
                VERBATIM)
            if (ARGC EQUAL 2)
                set(${ARGV1} TRUE PARENT_SCOPE)
            endif()
        endif()
    endif()
endfunction()
