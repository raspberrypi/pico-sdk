/*
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_XIP_CACHE_H
#define _HARDWARE_XIP_CACHE_H

#include "pico.h"
#include "hardware/regs/addressmap.h"

/** \file xip_cache.h
 *  \defgroup hardware_xip_cache hardware_xip_cache
 *
 * \brief Low-level cache maintenance operations for the XIP cache
 *
 * These functions apply some maintenance operation to either the entire cache contents, or a range
 * of offsets within the downstream address space. Offsets start from 0 (indicating the first byte
 * of flash), so pointers should have XIP_BASE subtracted before passing into one of these
 * functions.
 *
 * \if rp2040_specific
 * The only valid cache maintenance operation on RP2040 is "invalidate", which tells the cache to
 * forget everything it knows about some address. This is necessary after a programming operation,
 * because the cache does not automatically know about any serial programming operations performed
 * on the external flash device, and could return stale data.
 * \endif
 *
 * \if rp2350_specific
 * On RP2350, the three types of operation are:
 *
 * * Invalidate: tell the cache to forget everything it knows about some address. The next access to
 *   that address will fetch from downstream memory.
 *
 * * Clean: if the addressed cache line contains data not yet written to external memory, then write
 *   that data out now, and mark the line as "clean" (i.e. not containing uncommitted write data)
 *
 * * Pin: mark an address as always being resident in the cache. This persists until the line is
 *   invalidated, and can be used to allocate part of the cache for cache-as-SRAM use.
 *
 * When using both external flash and external RAM (e.g. PSRAM), a simple way to maintain coherence
 * over flash programming operations is to:
 *
 * 1. Clean the entire cache (e.g. using xip_cache_clean_all())
 *
 * 2. Erase + program the flash using serial SPI commands
 *
 * 3. Invalidate ("flush") the entire cache (e.g. using xip_cache_invalidate_all())
 *
 * The invalidate ensures the programming is visible to subsequent reads. The clean ensures that the
 * invalidate does not discard any cached PSRAM write data.
 *
 * \endif
 *
 */

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_HARDWARE_XIP_CACHE, Enable/disable assertions in the hardware_xip_cache module, type=bool, default=0, group=hardware_xip_cache
#ifndef PARAM_ASSERTIONS_ENABLED_HARDWARE_XIP_CACHE
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_XIP_CACHE 0
#endif

#define XIP_CACHE_LINE_SIZE _u(8)

#define XIP_CACHE_SIZE (_u(16) * _u(1024))

#if PICO_RP2040
#define XIP_CACHE_ADDRESS_SPACE_SIZE (_u(16) * _u(1024) * _u(1024))
#else
#define XIP_CACHE_ADDRESS_SPACE_SIZE (XIP_END - XIP_BASE)
#endif

// A read-only cache never requires cleaning (you can still call the functions, they are just no-ops)
#if PICO_RP2040
#define XIP_CACHE_IS_READ_ONLY 1
#else
#define XIP_CACHE_IS_READ_ONLY 0
#endif

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Invalidate the cache for the entire XIP address space
 *  \ingroup hardware_xip_cache
 *
 * Invalidation ensures that subsequent reads will fetch data from the downstream memory, rather
 * than using (potentially stale) cached data.
 *
 * This function is faster than calling xip_cache_invalidate_range() for the entire address space,
 * because it iterates over cachelines instead of addresses.
 *
 * @note Any pending write data held in the cache is lost: you can force the cache to commit these
 *  writes first, by calling xip_cache_clean_all()
 *
 * @note Unlike flash_flush_cache(), this function affects *only* the cache line state.
 *  flash_flush_cache() calls a ROM API which can have other effects on some platforms, like
 *  cleaning up the bootrom's QSPI GPIO setup on RP2040. Prefer this function for general cache
 *  maintenance use, and prefer flash_flush_cache in sequences of ROM flash API calls.
 */
void xip_cache_invalidate_all(void);

/*! \brief Invalidate a range of offsets within the XIP address space
 *  \ingroup hardware_xip_cache
 *
 * \param start_offset The first offset to be invalidated. Offset 0 means the first byte of XIP
 *  memory (e.g. flash). Pointers must have XIP_BASE subtracted before passing into this function.
 *  Must be 4-byte-aligned on RP2040. Must be a aligned to the start of a cache line
 *  (XIP_CACHE_LINE_SIZE) on other platforms.
 *
 * \param size_bytes The number of bytes to invalidate. Must be a multiple of 4 bytes on RP2040.
 *  Must be a multiple of XIP_CACHE_LINE_SIZE on other platforms.
 *
 * Invalidation ensures that subsequent reads will fetch data from the downstream memory, rather
 * than using (potentially stale) cached data.

 * @note Any pending write data held in the cache is lost: you can force the cache to commit these
 *  writes first, by calling xip_cache_clean_range() with the same parameters. Generally this is
 *  not necessary because invalidation is used with flash (write-behind via programming), and
 *  cleaning is used with PSRAM (writing through the cache).
 *
 */
void xip_cache_invalidate_range(uintptr_t start_offset, uintptr_t size_bytes);

#if !XIP_CACHE_IS_READ_ONLY || PICO_COMBINED_DOCS

/*! \brief Clean the cache for the entire XIP address space
 *  \ingroup hardware_xip_cache
 *
 * This causes the cache to write out all pending write data to the downstream memory. For example,
 * when suspending the system with state retained in external PSRAM, this ensures all data has made
 * it out to external PSRAM before powering down.
 *
 * This function is faster than calling xip_cache_clean_range() for the entire address space,
 * because it iterates over cachelines instead of addresses.
 *
 * \if rp2040_specific
 * On RP2040 this is a no-op, as the XIP cache is read-only. This is indicated by the
 * XIP_CACHE_IS_READ_ONLY macro.
 * \endif
 *
 * \if rp2350_specific
 * On RP2350, due to the workaround applied for RP2350-E11, this function also effectively
 * invalidates all cache lines after cleaning them. The next access to each line will miss. Avoid
 * this by calling xip_cache_clean_range() which does not suffer this issue.
 * \endif
 *
 */
void xip_cache_clean_all(void);

/*! \brief Clean a range of offsets within the XIP address space
 *  \ingroup hardware_xip_cache
 *
 * This causes the cache to write out pending write data at these offsets to the downstream memory.
 *
 * \if rp2040_specific
 * On RP2040 this is a no-op, as the XIP cache is read-only. This is indicated by the
 * XIP_CACHE_IS_READ_ONLY macro.
 * \endif
 *
 * \param start_offset The first offset to be invalidated. Offset 0 means the first byte of XIP
 *  memory (e.g. flash). Pointers must have XIP_BASE subtracted before passing into this function.
 *  Must be aligned to the start of a cache line (XIP_CACHE_LINE_SIZE).
 *
 * \param size_bytes The number of bytes to clean. Must be a multiple of XIP_CACHE_LINE_SIZE.
 */
void xip_cache_clean_range(uintptr_t start_offset, uintptr_t size_bytes);

#else
// Stub these out inline to avoid generating a call to an empty function when they are no-ops
static inline void xip_cache_clean_all(void) {}
static inline void xip_cache_clean_range(uintptr_t start_offset, uintptr_t size_bytes) {
    (void)start_offset;
    (void)size_bytes;
}
#endif

#if !PICO_RP2040 || PICO_COMBINED_DOCS

/*! \brief Pin a range of offsets within the XIP address space
 *  \ingroup hardware_xip_cache
 *
 * Pinning a line at an address allocates the line exclusively for use at that address. This means
 * that all subsequent accesses to that address will hit the cache, and will not go to downstream
 * memory. This persists until one of two things happens:
 *
 * * The line is invalidated, e.g. via xip_cache_invalidate_all()
 *
 * * The same line is pinned at a different address (note lines are selected by address modulo
 *   XIP_CACHE_SIZE)
 *
 * \param start_offset The first offset to be pinnned. Offset 0 means the first byte of XIP
 *  memory (e.g. flash). Pointers must have XIP_BASE subtracted before passing into this function.
 *  Must be aligned to the start of a cache line (XIP_CACHE_LINE_SIZE).
 *
 * \param size_bytes The number of bytes to pin. Must be a multiple of XIP_CACHE_LINE_SIZE.
 *
 */
void xip_cache_pin_range(uintptr_t start_offset, uintptr_t size_bytes);
#endif

#ifdef __cplusplus
}
#endif

#endif // !__ASSEMBLER__

#endif // !_HARDWARE_XIP_CACHE_H
