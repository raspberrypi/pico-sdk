/*

    We use this round-about way of including the Segger RTT source file
    because we need to suppress the "cast-align" and "cast-qual" warnings,
    and cmake will not let us change the COMPILE_OPTIONS for a single
    source file if the target has one source file in the current directory tree
    and another source file in a directory outside the current directory tree.

    See https://gitlab.kitware.com/cmake/cmake/-/issues/20128 for more details.

 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "../../../lib/rtt/RTT/SEGGER_RTT.c"
#pragma GCC diagnostic pop
