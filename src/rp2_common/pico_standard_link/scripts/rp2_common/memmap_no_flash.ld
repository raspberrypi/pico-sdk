/* Include platform memory locations */
INCLUDE "set_memory_locations.ld"

/* Include memory regions used */
INCLUDE "memory_ram.ld"
INCLUDE "memory_scratch.ld"

/* Include aliases for no_flash storage memory regions (alias to themselves) */
INCLUDE "memory_aliases_no_flash.ld"

/* Define entry point symbol */
ENTRY(_entry_point)

/* Include no_flash sections */
INCLUDE "sections_no_flash.ld"
