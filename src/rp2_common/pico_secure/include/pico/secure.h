/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_SECURE_H
#define _PICO_SECURE_H

#include "pico.h"
#include "pico/bootrom.h"

// PICO_CONFIG: PICO_NONSECURE_DMA_MAX_CHANNEL, Max number of DMA channels that can be allocated to non-secure use, type=int, default=NUM_DMA_CHANNELS, group=hardware_dma
#ifndef PICO_NONSECURE_DMA_MAX_CHANNEL
#define PICO_NONSECURE_DMA_MAX_CHANNEL NUM_DMA_CHANNELS
#endif

// PICO_CONFIG: PICO_NONSECURE_USER_IRQ_MIN, Lowest number user IRQ that can be allocated to non-secure use, type=int, default=FIRST_USER_IRQ, group=hardware_irq
#ifndef PICO_NONSECURE_USER_IRQ_MIN
#define PICO_NONSECURE_USER_IRQ_MIN FIRST_USER_IRQ
#endif

// PICO_CONFIG: PICO_NONSECURE_PIO_MAX, Max number of PIOs that can be allocated to non-secure use, type=int, default=NUM_PIOS, group=hardware_pio
#ifndef PICO_NONSECURE_PIO_MAX
#define PICO_NONSECURE_PIO_MAX NUM_PIOS
#endif

#ifdef __cplusplus
extern "C" {
#endif


#if PICO_SECURE || PICO_COMBINED_DOCS
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


// PICO_CONFIG: PICO_MAX_SECURE_CALL_USER_CALLBACKS, Maximum number of secure call user callbacks, default=4, advanced=true, group=pico_bootrom
#ifndef PICO_MAX_SECURE_CALL_USER_CALLBACKS
#define PICO_MAX_SECURE_CALL_USER_CALLBACKS 4
#endif

/*! Callback function type for user handled rom_secure_call
 *  \ingroup pico_bootrom
 */
typedef int (*rom_secure_call_callback_t)(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t fn);

/*!
 * \brief Add user ROM callback function
 * \ingroup pico_bootrom
 * 
 * Add a user callback for rom_secure_call called if using a function code starting with `0b1xxx`,
 * which is a "unique" or "private" function as specified by the rom_secure_call() documentation.
 * 
 * \param callback pointer to the callback function
 * \param fn_mask first 16 bits of fn codes this callback handles
 */
int rom_secure_call_add_user_callback(rom_secure_call_callback_t callback, uint16_t fn_mask);

/*!
 * \brief Remove user ROM callback function
 * \ingroup pico_bootrom
 * 
 * Remove a user callback for rom_secure_call which was previously added with rom_secure_call_add_user_callback()
 * 
 * \param callback pointer to the callback function
 */
void rom_secure_call_remove_user_callback(rom_secure_call_callback_t callback);
#endif // PICO_SECURE || PICO_COMBINED_DOCS

#if (PICO_ALLOW_NONSECURE_DMA && PICO_NONSECURE) || PICO_COMBINED_DOCS
/*! \brief Request unused dma channels from secure
 *  \ingroup hardware_dma
 *
 * \param num_channels the number of channels to request
 * \return the number of channels provided
 */
int dma_request_unused_channels_from_secure(int num_channels);
#endif

#if (PICO_ALLOW_NONSECURE_USER_IRQ && PICO_NONSECURE) || PICO_COMBINED_DOCS
/*! \brief Request unused user IRQs from secure
 *  \ingroup hardware_irq
 *
 * \param num_irqs the number of IRQs to request
 * \return the number of IRQs provided
 */
int user_irq_request_unused_from_secure(int num_irqs);
#endif

#if (PICO_ALLOW_NONSECURE_PIO && PICO_NONSECURE) || PICO_COMBINED_DOCS
/*! \brief Request unused PIO from secure
 *  \ingroup hardware_pio
 *
 * \return the PIO number
 */
int pio_request_unused_pio_from_secure(void);
#endif

#if (PICO_ALLOW_NONSECURE_RESETS && PICO_SECURE) || PICO_COMBINED_DOCS
#include "hardware/regs/resets.h"

// PICO_CONFIG: PICO_ALLOW_NONSECURE_RESETS_MASK, Mask of RESETS that can be accessed by non-secure, type=int, default=resets needed for other PICO_ALLOW_NONSECURE_* options, group=hardware_resets
#ifndef PICO_ALLOW_NONSECURE_RESETS_MASK
#define PICO_ALLOW_NONSECURE_RESETS_MASK (PICO_ALLOW_NONSECURE_USB ? RESETS_RESET_USBCTRL_BITS : 0)
#endif
#endif


#ifdef __cplusplus
}
#endif
#endif
