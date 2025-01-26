#include <stdio.h>

#if PICO_RP2040
#include "RP2040.h"
#else
#include "RP2350.h"
#endif
#include "pico/stdlib.h"
#include "hardware/irq.h"

__STATIC_FORCEINLINE int some_function(int i) {
    return __CLZ(i);
}

static bool pendsv_called, irq_handler_called;

void PendSV_Handler(void) {
    pendsv_called = true;
}

void DMA_IRQ_0_Handler(void) {
    irq_handler_called = true;
    irq_clear(DMA_IRQ_0_IRQn);
}

int main(void) {
    stdio_init_all();
    for(int i=0;i<10;i++) {
        printf("%d %d\n", i, some_function(i));
    }
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    printf("PENDSV: ");
    puts(pendsv_called ? "SUCCESS" : "FAILURE");
    printf("DMA_IRQ_0: ");
    irq_set_enabled(DMA_IRQ_0_IRQn, true);
    irq_set_pending(DMA_IRQ_0_IRQn);
    puts(irq_handler_called ? "SUCCESS" : "FAILURE");
    bool ok = pendsv_called && irq_handler_called;
    puts(ok ? "PASSED" : "FAILED");
    return !ok;
}
