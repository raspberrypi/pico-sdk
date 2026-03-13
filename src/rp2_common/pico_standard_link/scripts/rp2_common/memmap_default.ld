/* Include platform memory locations */
INCLUDE "rp2_common/set_memory_locations.ld"

/* Include memory regions used */
INCLUDE "rp2_common/memory_flash.ld"
INCLUDE "rp2_common/memory_ram.ld"
INCLUDE "rp2_common/memory_scratch.ld"

/* Include aliases for storage memory regions */
INCLUDE "rp2_common/memory_aliases_default.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include default sections */
INCLUDE "rp2_common/sections_default.ld"
