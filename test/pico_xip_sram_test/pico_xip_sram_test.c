#include <stdio.h>
#include "pico/stdlib.h"


int main(void) {
    stdio_init_all();
    printf("pico_xip_sram_test begins\n");

    for (int i = 0; i < 5; i++) {
        printf("running... %d\n", i);
        sleep_ms(500);
    }

    printf("pico_xip_sram_test ends\n");
    return 0;
}
