/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_PLATFORM_SECTION_MACROS_H
#define _PICO_PLATFORM_SECTION_MACROS_H

#ifndef __ASSEMBLER__

/*! \brief Section attribute macro for placement in RAM after the `.data` section
 *  \ingroup pico_platform
 *
 * For example a 400 element `uint32_t` array placed after the .data section
 *
 *     uint32_t __after_data("my_group_name") a_big_array[400];
 *
 * The section attribute is `.after_data.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __after_data
#define __after_data(group) __attribute__((section(".after_data." group)))
#endif

/*! \brief Section attribute macro for placement in RAM
 *  \ingroup pico_platform
 *
 * For example a 3 element `uint32_t` array placed in RAM (even though it is `static const`)
 *
 *     static const uint32_t __not_in_flash("my_group_name") an_array[3];
 *
 * The section attribute is `.time_critical.<group>`, which is used to maintain compatibility
 * with older linker scripts
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_ram
#define __in_ram(group) __attribute__((section(".time_critical." group)))
#endif

/*! \brief Section attribute macro for placement in the penultimate SRAM bank (known as "scratch X")
 *  \ingroup pico_platform
 *
 * Scratch X is commonly used for critical data and functions accessed only by one core (when only
 * one core is accessing the RAM bank, there is no opportunity for stalls)
 *
 * For example a `uint32_t` variable placed in "scratch X"
 *
 *     uint32_t __in_scratch_x("my_group_name") foo = 23;
 *
 * The section attribute is `.scratch_x.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_scratch_x
#define __in_scratch_x(group) __attribute__((section(".scratch_x." group)))
#endif

/*! \brief Section attribute macro for placement in the penultimate SRAM bank (known as "scratch X")
 *  \ingroup pico_platform
 *
 * Alias for \ref __in_scratch_x
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __scratch_x
#define __scratch_x(group) __in_scratch_x(group)
#endif

/*! \brief Section attribute macro for placement in the final SRAM bank (known as "scratch Y")
 *  \ingroup pico_platform
 *
 * Scratch Y is commonly used for critical data and functions accessed only by one core (when only
 * one core is accessing the RAM bank, there is no opportunity for stalls)
 *
 * For example a `uint32_t` variable placed in "scratch Y"
 *
 *     uint32_t __scratch_y("my_group_name") foo = 23;
 *
 * The section attribute is `.scratch_y.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_scratch_y
#define __in_scratch_y(group) __attribute__((section(".scratch_y." group)))
#endif

/*! \brief Section attribute macro for placement in the final SRAM bank (known as "scratch Y")
 *  \ingroup pico_platform
 *
 * Alias for \ref __in_scratch_y
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __scratch_y
#define __scratch_y(group) __in_scratch_y(group)
#endif

/*! \brief Section attribute macro for placement in PSRAM
 *  \ingroup pico_platform
 *
 * PSRAM is commonly used for extra data sections. You can place data in initialised or
 * uninitialised PSRAM, depending on how the data is loaded into the PSRAM.
 *
 * For example a `uint32_t` variable placed in PSRAM
 *
 *     uint32_t __in_psram("my_group_name") foo = 23;
 *
 * Or placed in uninitialised PSRAM
 *
 *     uint32_t __uninitialized_psram("my_group_name") foo;
 *
 * The section attribute is `.psram_initialised.<group>` or `.psram_uninitialised.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_psram
#define __in_psram(group) __attribute__((section(".psram_initialised." group)))
#endif

/*! \brief Section attribute macro for placement in uninitialised PSRAM
 *  \ingroup pico_platform
 *
 * The version of \ref __in_psram to use for uninitialised data
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __uninitialized_psram
#define __uninitialized_psram(group) __attribute__((section(".psram_uninitialised." group)))
#endif

/*! \brief Section attribute macro for placement in XIP SRAM
 *  \ingroup pico_platform
 *
 * The XIP Cache can be used as SRAM for extra data sections, however it will give a performance
 * penalty if your binary runs from Flash (e.g. the default binary type).
 *
 * For example a `uint32_t` variable placed in XIP SRAM
 *
 *     uint32_t __in_xip_ram("my_group_name") foo = 23;
 *
 * The section attribute is `.xip_ram.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_xip_ram
#if PICO_USE_XIP_CACHE_AS_RAM
#define __in_xip_ram(group) __attribute__((section(".xip_ram." group)))
#elif PICO_XIP_RAM
#define __in_xip_ram(group) __in_ram(group)
#else
#define __in_xip_ram(group) x; static_assert(false, "Must set PICO_USE_XIP_CACHE_AS_RAM=1 to use the __in_xip_ram macro");
#endif
#endif

/*! \brief Section attribute macro for data that is to be left uninitialized
 *  \ingroup pico_platform
 *
 * Data marked this way will retain its value across a reset (normally uninitialized data - in the .bss
 * section) is initialized to zero during runtime initialization
 *
 * For example a `uint32_t` foo that will retain its value if the program is restarted by reset.
 *
 *     uint32_t __uninitialized_ram(foo);
 *
 * The section attribute is `.uninitialized_data.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __uninitialized_ram
#define __uninitialized_ram(group) __attribute__((section(".uninitialized_data." #group))) group
#endif

#if LIB_PICO_LOW_POWER && HAS_POWMAN_TIMER
/*! \brief Section attribute macro for placement in a section persisted across default POWMAN resets
 *  \ingroup pico_platform
 *
 * Data marked this way will retain its value across a default POWMAN reset, and will be zeroed on
 * any other reset.
 *
 * For example a `uint32_t` foo that will be zeroed initially, then retain its value if the program
 * is restarted by default POWMAN reset.
 *
 *     uint32_t __persistent_data(foo);
 *
 * The section attribute is `.persistent_data.<name>`
 *
 * \param name  the name of the variable to place in the section
 */
#ifndef __persistent_data
#define __persistent_data(name) __attribute__((section(".persistent_data." #name))) name
#endif
#else
// If not supported, use the .bss section, as that will be zeroed on boot
#ifndef __persistent_data
#define __persistent_data(name) __attribute__((section(".bss." #name))) name
#endif
#endif

/*! \brief Section attribute macro for placement in flash even in a COPY_TO_RAM binary
 *  \ingroup pico_platform
 *
 * For example a `uint32_t` variable explicitly placed in flash (it will hard fault if you attempt to write it!)
 *
 *     uint32_t __in_flash("my_group_name") foo = 23;
 *
 * The section attribute is `.flashdata.<group>`
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef __in_flash
#define __in_flash(group) __attribute__((section(".flashdata." group)))
#endif

/*! \brief Section attribute macro for placement not in flash
 *  \ingroup pico_platform
 *
 * For example a 3 element `uint32_t` array placed in RAM (even though it is `static const`)
 *
 *     static const uint32_t __not_in_flash("my_group_name") an_array[3];
 *
 * By default, this is identical to \ref __in_ram, but this can be adjusted using the `PICO_NOT_IN_FLASH_PLACEMENT` define.
 * This define can be set using the \ref pico_set_not_in_flash_placement CMake function.
 *
 * For example, for binaries that only use core 0, there is the option to use
 * `pico_set_not_in_flash_placement(TARGET scratch_x)` to place this code/data in scratch X to move it
 * out of the striped SRAM.
 *
 * \param group a string suffix to use in the section name to distinguish groups that can be linker
 *              garbage-collected independently
 */
#ifndef PICO_NOT_IN_FLASH_PLACEMENT
#define PICO_NOT_IN_FLASH_PLACEMENT __in_ram
#endif
#ifndef __not_in_flash
#define __not_in_flash(group) PICO_NOT_IN_FLASH_PLACEMENT(group)
#endif

/*! \brief Indicates a function should not be stored in flash
 *  \ingroup pico_platform
 *
 * Decorates a function name, such that the function will execute from RAM (assuming it is not inlined
 * into a flash function by the compiler)
 *
 * For example a function called my_func taking an int parameter:
 *
 *     void __not_in_flash_func(my_func)(int some_arg) {
 *
 * The function is placed using \ref __not_in_flash, which defaults to \ref __in_ram
 *
 * \see __no_inline_not_in_flash_func
 */
#ifndef __not_in_flash_func
#define __not_in_flash_func(func_name) __not_in_flash(__STRING(func_name)) func_name
#endif

/*! \brief Indicate a function should not be stored in flash and should not be inlined
 *  \ingroup pico_platform
 *
 * Decorates a function name, such that the function will execute from RAM, explicitly marking it as
 * noinline to prevent it being inlined into a flash function by the compiler
 *
 * For example a function called my_func taking an int parameter:
 *
 *     void __no_inline_not_in_flash_func(my_func)(int some_arg) {
 *
 * The function is placed using \ref __not_in_flash, which defaults to \ref __in_ram
 */
#ifndef __no_inline_not_in_flash_func
#define __no_inline_not_in_flash_func(func_name) __noinline __not_in_flash_func(func_name)
#endif


/*! \brief Indicates a function is time/latency critical and should not run from flash
 *  \ingroup pico_platform
 *
 * Decorates a function name, such that the function will execute from RAM to avoid possible flash latency. By default,
 * this macro is identical in implementation to `__no_inline_not_in_flash_func`, however the semantics are distinct and
 * a `__time_critical_func` can be treated more specially to reduce the overhead when calling such a function.
 *
 * For example a function called my_func taking an int parameter:
 *
 *     void __time_critical_func(my_func)(int some_arg) {
 *
 * By default, the function is placed using \ref __in_ram, but this can be adjusted using the
 * `PICO_TIME_CRITICAL_PLACEMENT` define. This define can be set using the \ref pico_set_time_critical_placement
 * CMake function.
 * 
 * For example, for binaries that are not executing from flash (e.g. copy_to_ram and no_flash), there is the option
 * to use `pico_set_time_critical_placement(TARGET xip_ram)` to place these functions in XIP RAM, as the XIP AHB
 * ports would be otherwise unused.
 *
 * \see __not_in_flash
 */
#ifndef __time_critical_func
#ifndef PICO_TIME_CRITICAL_PLACEMENT
#define PICO_TIME_CRITICAL_PLACEMENT __in_ram
#endif
#define __time_critical_func(func_name) __noinline PICO_TIME_CRITICAL_PLACEMENT(__STRING(func_name)) func_name
#endif

#else

#ifndef RAM_SECTION_NAME
#define RAM_SECTION_NAME(x) .time_critical.##x
#endif

#ifndef SECTION_NAME
#define SECTION_NAME(x) .text.##x
#endif

#endif // !__ASSEMBLER__

#endif

