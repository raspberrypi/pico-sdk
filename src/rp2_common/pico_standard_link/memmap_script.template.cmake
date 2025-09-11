# These lines are configured to be set(RAM_ORIGIN xxx) set(RAM_LENGTH xxx) etc.
@RAM@
@SCRATCH_X@
@SCRATCH_Y@
@ADDITIONAL_MEMORY@
@ADDITIONAL_PRE_DATA@

# Set some defaults
if (NOT FLASH_REGION)
    set(FLASH_REGION "INCLUDE \"pico_flash_region.ld\"")
endif()

configure_file("${PICO_LINKER_SCRIPT_PATH}/memmap_@TYPE@.template.ld" "${output_file}" @ONLY)
