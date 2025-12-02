/* Include memory regions used */
INCLUDE "rp2_common/memory_flash.ld"
INCLUDE "platform/memory_ram.ld"
INCLUDE "platform/memory_scratch.ld"

/* Include aliases for storage memory regions */
INCLUDE "rp2_common/memory_aliases_default.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include default sections */
INCLUDE "rp2_common/sections_copy_to_ram.ld"
