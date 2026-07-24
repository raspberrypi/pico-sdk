/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/flash_image.h"
#include "debug_uart_tx.pio.h"

// If the output GPIO is defined, then we can use the PIO UART to log messages.
#ifdef DBG_PIO_UART_OUTPUT_GPIO

#define PIO_UART_BAUDRATE               115200

// In the interests of speed, longer messages are cropped
#define DEBUG_MSG_LOG_BUFFER_LENGTH     120

typedef struct {
    int  initialised;
    int  bytes_in_word;
    char printf_buffer[DEBUG_MSG_LOG_BUFFER_LENGTH];
} flashlog_t;

static flashlog_t flashlog = { 0 };

// Define PIO engine and state machine instances
static const PIO PIO_UART_PIO = pio0;
static const uint SM0 = 0;


static void pio_uart_initialise(void) {

    flashlog.bytes_in_word = 0;

    uint offset = pio_add_program(PIO_UART_PIO, &uart_tx_program);

    // Tell PIO to initially drive output-high on the selected pin, then map PIO
    // onto that pin with the IO muxes.
    pio_sm_set_pins_with_mask(PIO_UART_PIO, SM0, 1u << DBG_PIO_UART_OUTPUT_GPIO, 1u << DBG_PIO_UART_OUTPUT_GPIO);
    pio_sm_set_pindirs_with_mask(PIO_UART_PIO, SM0, 1u << DBG_PIO_UART_OUTPUT_GPIO, 1u << DBG_PIO_UART_OUTPUT_GPIO);
    pio_gpio_init(PIO_UART_PIO, DBG_PIO_UART_OUTPUT_GPIO);

    pio_sm_config c = uart_tx_program_get_default_config(offset);

    // OUT shifts to right, no autopull
    sm_config_set_out_shift(&c, true, false, 32);

    // We are mapping both OUT and side-set to the same pin, because sometimes
    // we need to assert user data onto the pin (with OUT) and sometimes
    // assert constant values (start/stop bit)
    sm_config_set_out_pins(&c, DBG_PIO_UART_OUTPUT_GPIO, 1);
    sm_config_set_sideset_pins(&c, DBG_PIO_UART_OUTPUT_GPIO);

    // We only need TX, so get an 8-deep FIFO!
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // SM transmits 1 bit per 8 execution cycles.
    float div = (float)clock_get_hz(clk_sys) / (8 * PIO_UART_BAUDRATE);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(PIO_UART_PIO, SM0, offset, &c);
    pio_sm_set_enabled(PIO_UART_PIO, SM0, true);
}


// NOTE! There is a sleight of hand in respect of the FIFO pack/unpacking.
// 0x00 is used as a FIFO word padder during flushing and the PIO program will
// wait on a new word from the FIFO *once* the Output Shift Register is 0
// but only *after* the first character from the FIFO has been transmitted.
// If you need to send arbitrarily placed 0x00 characters, this code will need
// to send those as the first character in a FIFO word and pad the rest of the
// word with zeros (or non-zero data if that is next) to ensure they get sent.
static void __attribute__((noinline)) pio_uart_putchar(char c) {

    static union { uint32_t w; char c[4]; } __attribute__((__packed__)) bytes_word;

    bytes_word.c[flashlog.bytes_in_word++] = c;

    if (flashlog.bytes_in_word > 3) {
        pio_sm_put_blocking(PIO_UART_PIO, SM0, bytes_word.w);
        flashlog.bytes_in_word = 0;
    }
}


static void pio_uart_flush(void) {

    while (0 != flashlog.bytes_in_word) {
        pio_uart_putchar(0);
    }
}


static void pio_uart_send_buffer(char *data_ptr, int data_len) {

    if (NULL == data_ptr) {
        return;
    }

    // Note: This becomes a blocking busy loop once the FIFO is
    // full, see comments for `DBG_PIO_UART_MAX_FAST_LOG_LEN`
    while (data_len--)  {
        char c = *data_ptr++;
        if (0x0a == c) { // convert LFs into CR+LF
            pio_uart_putchar(0x0d);
        }
        pio_uart_putchar(c);
    }

    // With FIFO packing we need a flush here!
    pio_uart_flush();
}

//----------------------------------------------------------------------------

void flashlog_printf(const char *fmt, ...) {

    if (!flashlog.initialised) {
        pio_uart_initialise();
        flashlog.initialised = 1;
    }

    int data_len = 0;
    va_list args;

    if (fmt) {
        va_start(args, fmt);
        data_len = vsnprintf(flashlog.printf_buffer, sizeof(flashlog.printf_buffer), fmt, args);
        va_end(args);
    }

    pio_uart_send_buffer(flashlog.printf_buffer, data_len);
}

#endif // DBG_PIO_UART_OUTPUT_GPIO