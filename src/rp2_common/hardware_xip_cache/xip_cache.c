#include "hardware/xip_cache.h"
#include "hardware/structs/xip.h"
// For barrier macros:
#include "hardware/sync.h"

// Implementation-private constants (exporting these would create a compatibility headache as they
// don't exist on all platforms; all of these operations are exposed through APIs anyways)

#if !PICO_RP2040
typedef enum {
    XIP_CACHE_INVALIDATE_BY_SET_WAY = 0,
    XIP_CACHE_CLEAN_BY_SET_WAY = 1,
    XIP_CACHE_INVALIDATE_BY_ADDRESS = 2,
    XIP_CACHE_CLEAN_BY_ADDRESS = 3,
    XIP_CACHE_PIN_AT_ADDRESS = 7,
    XIP_CACHE_OP_MAX = 7
} cache_op_t;
#endif

// Used to ensure subsequent accesses observe the new state of the maintained cache lines
#define __post_maintenance_barrier() do {__dsb(); __isb();} while (0)

// All functions in this file are marked non-flash, even though they themselves may be executed
// safely from flash, because they are likely to be called during a flash programming operation
// (which makes flash execution momentarily unsafe)

__force_inline static void check_xip_offset_range(uintptr_t start_offset, uintptr_t size_bytes) {
    // We use offsets, not addresses, for consistency with the flash API. This means the range of
    // valid inputs starts at 0.
    (void)start_offset;
    (void)size_bytes;
    valid_params_if(HARDWARE_XIP_CACHE, start_offset <= XIP_CACHE_ADDRESS_SPACE_SIZE);
    valid_params_if(HARDWARE_XIP_CACHE, start_offset + size_bytes <= XIP_CACHE_ADDRESS_SPACE_SIZE);
    // Check for unsigned wrapping too:
    valid_params_if(HARDWARE_XIP_CACHE, start_offset + size_bytes >= start_offset);
}

#if !PICO_RP2040
// Generic code for RP2350-style caches: apply a maintenance operation to a range of offsets
static void __no_inline_not_in_flash_func(xip_cache_maintain)(uintptr_t start_offset, uintptr_t size_bytes, cache_op_t op) {
    check_xip_offset_range(start_offset, size_bytes);
    valid_params_if(HARDWARE_XIP_CACHE, (start_offset & (XIP_CACHE_LINE_SIZE - 1u)) == 0);
    valid_params_if(HARDWARE_XIP_CACHE, (size_bytes & (XIP_CACHE_LINE_SIZE - 1u)) == 0);
    valid_params_if(HARDWARE_XIP_CACHE, (uint)op <= (uint)XIP_CACHE_OP_MAX);

    uintptr_t end = start_offset + size_bytes;
    for (uintptr_t offset = start_offset; offset < end; offset += XIP_CACHE_LINE_SIZE) {
        *(io_wo_8 *) (XIP_MAINTENANCE_BASE + offset + (uintptr_t)op) = 0;
    }
    __post_maintenance_barrier();
}
#endif

void __no_inline_not_in_flash_func(xip_cache_invalidate_all)(void) {
#if PICO_RP2040
    xip_ctrl_hw->flush = 1;
    // Read back to wait for completion
    (void)xip_ctrl_hw->flush;
    __post_maintenance_barrier();
#else
    xip_cache_maintain(XIP_CACHE_ADDRESS_SPACE_SIZE - XIP_CACHE_SIZE, XIP_CACHE_SIZE, XIP_CACHE_INVALIDATE_BY_SET_WAY);
#endif
}

void __no_inline_not_in_flash_func(xip_cache_invalidate_range)(uintptr_t start_offset, uintptr_t size_bytes) {
#if PICO_RP2040
    // Accsses are at intervals of one half cache line (so 4 bytes) because RP2040's cache has two
    // valid flags per cache line, and we need to clear both.
    check_xip_offset_range(start_offset, size_bytes);
    valid_params_if(HARDWARE_XIP_CACHE, (start_offset & 3u) == 0);
    valid_params_if(HARDWARE_XIP_CACHE, (size_bytes & 3u) == 0);

    uintptr_t end = start_offset + size_bytes;
    // On RP2040 you can invalidate a sector (half-line) by writing to its normal cached+allocating address
    for (uintptr_t offset = start_offset; offset < end; offset += 4u) {
        *(io_wo_32 *)(offset + XIP_BASE) = 0;
    }
    __post_maintenance_barrier();

#else

    xip_cache_maintain(start_offset, size_bytes, XIP_CACHE_INVALIDATE_BY_ADDRESS);

#endif
}

#if !XIP_CACHE_IS_READ_ONLY
void __no_inline_not_in_flash_func(xip_cache_clean_all)(void) {
    // Use addresses outside of the downstream QMI address range to work around RP2350-E11; this
    // effectively performs a clean+invalidate (except being a no-op on pinned lines) due to the
    // erroneous update of the tag. Consequently you will take a miss on the next access to the
    // cleaned address.
    xip_cache_maintain(XIP_END - XIP_BASE - XIP_CACHE_SIZE, XIP_CACHE_SIZE, XIP_CACHE_CLEAN_BY_SET_WAY);
}
#endif

#if !XIP_CACHE_IS_READ_ONLY
void __no_inline_not_in_flash_func(xip_cache_clean_range)(uintptr_t start_offset, uintptr_t size_bytes) {
    xip_cache_maintain(start_offset, size_bytes, XIP_CACHE_CLEAN_BY_ADDRESS);
}
#endif

#if !PICO_RP2040
void __no_inline_not_in_flash_func(xip_cache_pin_range)(uintptr_t start_offset, uintptr_t size_bytes) {
    valid_params_if(HARDWARE_XIP_CACHE, size_bytes <= XIP_CACHE_SIZE);
    xip_cache_maintain(start_offset, size_bytes, XIP_CACHE_PIN_AT_ADDRESS);
}
#endif

