# These lines are configured to be set(RAM_ORIGIN xxx) set(RAM_LENGTH xxx) etc.
@RAM@
@SCRATCH_X@
@SCRATCH_Y@

configure_file("${PICO_LINKER_SCRIPT_PATH}/memmap_@TYPE@.template.ld" "${output_file}" @ONLY)
