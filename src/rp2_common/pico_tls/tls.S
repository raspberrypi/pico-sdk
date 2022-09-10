#include "pico/asm_helper.S"

// This has to happen after memcpy() and memset() are available.
__pre_init tls_init, 10000
