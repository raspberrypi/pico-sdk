/* Include memory regions used */
INCLUDE "platform/memory_ram.ld"
INCLUDE "platform/memory_scratch.ld"

/* Include aliases for no_flash storage memory regions (alias to themselves) */
INCLUDE "rp2_common/memory_aliases_no_flash.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include no_flash sections */
INCLUDE "rp2_common/sections_no_flash.ld"
