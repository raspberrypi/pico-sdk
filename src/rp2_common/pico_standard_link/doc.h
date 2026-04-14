/**
 * \defgroup pico_standard_link pico_standard_link
 * \brief Setup for link options for a standard SDK executable
 * 
 * This includes:
 *   - C runtime initialization
 *   - Linker scripts for `default`, `no_flash`, `blocked_ram` and `copy_to_ram` binaries
 *   - 'Binary Information' support
 *   - Linker option control
 *
 * The default linker scripts comprise of a base `memmap_*.ld` file, and a series of
 * `.incl` files that are included by it.
 *
 * The base `memmap_*.ld` files are located in the `pico_platform` directory of the target chip.
 * Each file has a tree showing the files that are included by it.
 *
 * To override any included files, place the overriding files in a directory, and pass
 * that directory to the \ref pico_add_linker_script_override_path CMake function. The usual
 * files to override when making a custom linker script are:
 *   - `memory_extra.incl`, to add extra memory regions
 *   - `section_extra_*.incl` files, to add extra sections to the binary - see the comments in the
 *     files for more details
 *   - `default_*_excludes.incl` files, to change what is placed in RAM for flash binary types
 *     (eg `default`, `blocked_ram`)
 *
 * You can also set variables in the linker script using the \ref pico_set_linker_script_var CMake 
 * function, to change the location and size of default memory regions. The available variables are:
 *   - `HEAP_LOC`, to set the heap location
 *   - `HEAP_LIMIT`, to set the heap limit
 *   - `RAM_ORIGIN`, to set the origin of RAM
 *   - `RAM_LENGTH`, to set the length of RAM
 *   - `XIP_RAM_ORIGIN`, to set the origin of XIP RAM
 *   - `XIP_RAM_LENGTH`, to set the length of XIP RAM
 *   - `SCRATCH_X_ORIGIN`, to set the origin of scratch x
 *   - `SCRATCH_X_LENGTH`, to set the length of scratch x
 *   - `SCRATCH_Y_ORIGIN`, to set the origin of scratch y
 *   - `SCRATCH_Y_LENGTH`, to set the length of scratch y
 */
