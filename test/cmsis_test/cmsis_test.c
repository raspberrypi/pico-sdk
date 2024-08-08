#include <stdio.h>

#if PICO_RP2040
#include "RP2040.h"
#else
#include "RP2350.h"
#endif
#include "pico/stdio.h"

__STATIC_FORCEINLINE int some_function(int i) {
    return __CLZ(i);
}

static bool pendsv_called;

void PendSV_Handler(void) {
    pendsv_called = true;
}

int main(void) {
    stdio_init_all();
    for(int i=0;i<10;i++) {
        printf("%d %d\n", i, some_function(i));
    }
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    puts(pendsv_called ? "SUCCESS" : "FAILURE");
}
