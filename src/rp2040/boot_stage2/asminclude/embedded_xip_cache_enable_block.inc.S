#include "hardware/regs/addressmap.h"
#include "hardware/regs/xip.h"

#if defined(PICO_USE_XIP_CACHE_AS_RAM) && PICO_USE_XIP_CACHE_AS_RAM
    // Disable the XIP cache making its 16k of XIP RAM available for storage
_b2_disable_xip_cache:
    ldr r0, =(REG_ALIAS_CLR_BITS + XIP_CTRL_BASE + XIP_CTRL_OFFSET)
    movs r1, #XIP_CTRL_EN_BITS
    str r1, [r0]
#else
    // Enable the XIP cache (hardware default)
_b2_enable_xip_cache:
    ldr r0, =(REG_ALIAS_SET_BITS + XIP_CTRL_BASE + XIP_CTRL_OFFSET)
    movs r1, #XIP_CTRL_EN_BITS
    str r1, [r0]
#endif
