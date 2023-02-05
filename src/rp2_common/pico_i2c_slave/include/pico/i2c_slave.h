/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _I2C_SLAVE_H_
#define _I2C_SLAVE_H_

#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file pico/i2c_slave.h
 * \defgrup pico_i2c_slave pico_i2c_slave
 * \brief I2C slave helper library, which takes care of hooking the I2C IRQ and calling back the user with I2C events.
 */

/**
 * \brief I2C slave event types.
 */
typedef enum i2c_slave_event_t
{
    I2C_SLAVE_RECEIVE, /**< Data from master is available for reading. Slave must read from Rx FIFO. */
    I2C_SLAVE_REQUEST, /**< Master is requesting data. Slave must write into Tx FIFO. */
    I2C_SLAVE_FINISH, /**< Master has sent a Stop or Restart signal. Slave may prepare for the next transfer. */
} i2c_slave_event_t;

/**
 * \brief I2C slave event handler
 * 
 * The event handler will run from the I2C ISR, so it should return quickly (under 25 us at 400 kb/s).
 * Avoid blocking inside the handler and split large data transfers across multiple calls for best results.
 * When sending data to master, up to `i2c_get_write_available()` bytes can be written without blocking.
 * When receiving data from master, up to `i2c_get_read_available()` bytes can be read without blocking.
 * 
 * \param i2c Slave I2C instance.
 * \param event Event type.
 */
typedef void (*i2c_slave_handler_t)(i2c_inst_t *i2c, i2c_slave_event_t event);

/**
 * \brief Configure I2C instance for slave mode.
 * 
 * \param i2c I2C instance.
 * \param address 7-bit slave address.
 * \param handler Called on events from I2C master. It will run from the I2C ISR, on the CPU core
 *                where the slave was initialized.
 */
void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler);

/**
 * \brief Restore I2C instance to master mode.
 *
 * \param i2c I2C instance.
 */
void i2c_slave_deinit(i2c_inst_t *i2c);

#ifdef __cplusplus
}
#endif

#endif // _I2C_SLAVE_H_
