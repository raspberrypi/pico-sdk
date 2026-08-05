#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();

    // Ordinary use of errno. Under picolibc errno is thread-local, so this is a TLS
    // reference from a translation unit compiled by the SDK.
    errno = 0;
    long v = strtol("99999999999999999999", NULL, 10);
    printf("strtol -> %ld, errno=%d (ERANGE=%d)\n", v, errno, ERANGE);

    return 0;
}
