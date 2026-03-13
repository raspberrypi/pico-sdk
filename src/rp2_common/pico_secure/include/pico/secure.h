/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_SECURE_H
#define _PICO_SECURE_H

#include "pico.h"
#include "pico/bootrom.h"

#ifdef __cplusplus
extern "C" {
#endif


/*! \brief  Launch non-secure binary
 *  \ingroup pico_secure
 *
 * \note The secure binary must have already set it's stack limits, using PICO_USE_STACK_GUARDS or similar
 *
 * \param vtor_address The vector table address of the non-secure binary
 * \param stack_limit The stack limit of the non-secure binary
 */
void secure_launch_nonsecure_binary(uint32_t vtor_address, uint32_t stack_limit);

/*! \brief  Configure SAU region
 *  \ingroup pico_secure
 *
 * \param region The region to configure
 * \param base The base address of the region
 * \param limit The limit address of the region
 * \param enabled Whether the region is enabled
 * \param nsc Whether the region is non-secure callable
 */
void secure_sau_configure_region(uint region, uint32_t base, uint32_t limit, bool enabled, bool nsc);

#if defined(PICO_SECURITY_SPLIT_CONFIGURED)
/*! \brief  Configure default SAU regions
 *  \ingroup pico_secure
 *
 * Configures the default security split configuration, based on the split configured with pico_set_security_ram_split
 */
void secure_sau_configure_split(void);


/*! \brief  Launch non-secure binary from default location
 *  \ingroup pico_secure
 *
 * Launches non-secure binary from the default location, based on the split configured with pico_set_security_ram_split
 */
void secure_launch_nonsecure_binary_default(void);
#endif

/*! \brief  Set SAU enabled
 *  \ingroup pico_secure
 *
 * Set SAU enabled, with appropriate memory barriers
 *
 * \param enabled Whether the SAU is enabled
 */
void secure_sau_set_enabled(bool enabled);


typedef void (*secure_hardfault_callback_t)(void);

/*! \brief  Install default hardfault handler
 *  \ingroup pico_secure
 *
 * \param callback The callback to call when a hardfault occurs after printing the information
 */
void secure_install_default_hardfault_handler(secure_hardfault_callback_t callback);


#ifdef __cplusplus
}
#endif
#endif
