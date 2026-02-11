/* Include platform memory locations */
INCLUDE "set_memory_locations.ld"

/* Include memory regions used */
INCLUDE "memory_flash.ld"
INCLUDE "memory_ram.ld"
INCLUDE "memory_scratch.ld"

/* Include aliases for storage memory regions */
INCLUDE "memory_aliases_default.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include default sections */
INCLUDE "sections_default.ld"
