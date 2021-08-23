// THIS HEADER FILE IS AUTOMATICALLY GENERATED -- DO NOT EDIT

/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_SPI_H
#define _HARDWARE_STRUCTS_SPI_H

#include "hardware/address_mapped.h"
#include "hardware/regs/spi.h"

// reference to datasheet: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf#tab-registerlist_spi
// BITMASK : FIELDNAME [BITRANGE] (RESETVALUE): DESCRIPTION

typedef struct {
    _REG_(SPI_SSPCR0_OFFSET)
    // Control register 0, SSPCR0 on page 3-4
    // 0x0000ff00 [8-15]  : SCR (0): Serial clock rate
    // 0x00000080 [7]     : SPH (0): SSPCLKOUT phase, applicable to Motorola SPI frame format only
    // 0x00000040 [6]     : SPO (0): SSPCLKOUT polarity, applicable to Motorola SPI frame format only
    // 0x00000030 [4-5]   : FRF (0): Frame format: 00 Motorola SPI frame format
    // 0x0000000f [0-3]   : DSS (0): Data Size Select: 0000 Reserved, undefined operation
    io_rw_32 cr0;

    _REG_(SPI_SSPCR1_OFFSET)
    // Control register 1, SSPCR1 on page 3-5
    // 0x00000008 [3]     : SOD (0): Slave-mode output disable
    // 0x00000004 [2]     : MS (0): Master or slave mode select
    // 0x00000002 [1]     : SSE (0): Synchronous serial port enable: 0 SSP operation disabled
    // 0x00000001 [0]     : LBM (0): Loop back mode: 0 Normal serial port operation enabled
    io_rw_32 cr1;

    _REG_(SPI_SSPDR_OFFSET)
    // Data register, SSPDR on page 3-6
    // 0x0000ffff [0-15]  : DATA (0): Transmit/Receive FIFO: Read Receive FIFO
    io_rw_32 dr;

    _REG_(SPI_SSPSR_OFFSET)
    // Status register, SSPSR on page 3-7
    // 0x00000010 [4]     : BSY (0): PrimeCell SSP busy flag, RO: 0 SSP is idle
    // 0x00000008 [3]     : RFF (0): Receive FIFO full, RO: 0 Receive FIFO is not full
    // 0x00000004 [2]     : RNE (0): Receive FIFO not empty, RO: 0 Receive FIFO is empty
    // 0x00000002 [1]     : TNF (1): Transmit FIFO not full, RO: 0 Transmit FIFO is full
    // 0x00000001 [0]     : TFE (1): Transmit FIFO empty, RO: 0 Transmit FIFO is not empty
    io_ro_32 sr;

    _REG_(SPI_SSPCPSR_OFFSET)
    // Clock prescale register, SSPCPSR on page 3-8
    // 0x000000ff [0-7]   : CPSDVSR (0): Clock prescale divisor
    io_rw_32 cpsr;

    _REG_(SPI_SSPIMSC_OFFSET)
    // Interrupt mask set or clear register, SSPIMSC on page 3-9
    // 0x00000008 [3]     : TXIM (0): Transmit FIFO interrupt mask: 0 Transmit FIFO half empty or less...
    // 0x00000004 [2]     : RXIM (0): Receive FIFO interrupt mask: 0 Receive FIFO half full or less...
    // 0x00000002 [1]     : RTIM (0): Receive timeout interrupt mask: 0 Receive FIFO not empty and no...
    // 0x00000001 [0]     : RORIM (0): Receive overrun interrupt mask: 0 Receive FIFO written to while...
    io_rw_32 imsc;

    _REG_(SPI_SSPRIS_OFFSET)
    // Raw interrupt status register, SSPRIS on page 3-10
    // 0x00000008 [3]     : TXRIS (1): Gives the raw interrupt state, prior to masking, of the...
    // 0x00000004 [2]     : RXRIS (0): Gives the raw interrupt state, prior to masking, of the...
    // 0x00000002 [1]     : RTRIS (0): Gives the raw interrupt state, prior to masking, of the...
    // 0x00000001 [0]     : RORRIS (0): Gives the raw interrupt state, prior to masking, of the...
    io_ro_32 ris;

    _REG_(SPI_SSPMIS_OFFSET)
    // Masked interrupt status register, SSPMIS on page 3-11
    // 0x00000008 [3]     : TXMIS (0): Gives the transmit FIFO masked interrupt state, after masking,...
    // 0x00000004 [2]     : RXMIS (0): Gives the receive FIFO masked interrupt state, after masking, of...
    // 0x00000002 [1]     : RTMIS (0): Gives the receive timeout masked interrupt state, after masking,...
    // 0x00000001 [0]     : RORMIS (0): Gives the receive over run masked interrupt status, after...
    io_ro_32 mis;

    _REG_(SPI_SSPICR_OFFSET)
    // Interrupt clear register, SSPICR on page 3-11
    // 0x00000002 [1]     : RTIC (0): Clears the SSPRTINTR interrupt
    // 0x00000001 [0]     : RORIC (0): Clears the SSPRORINTR interrupt
    io_rw_32 icr;

    _REG_(SPI_SSPDMACR_OFFSET)
    // DMA control register, SSPDMACR on page 3-12
    // 0x00000002 [1]     : TXDMAE (0): Transmit DMA Enable
    // 0x00000001 [0]     : RXDMAE (0): Receive DMA Enable
    io_rw_32 dmacr;

    _REG_(SPI__OFFSET)
    uint32_t _pad0[1006];

    _REG_(SPI_None_OFFSET)
    io_ro_32 periphid[4];

    _REG_(SPI_None_OFFSET)
    io_ro_32 pcellid[4];

} spi_hw_t;

#define spi0_hw ((spi_hw_t *const)SPI0_BASE)
#define spi1_hw ((spi_hw_t *const)SPI1_BASE)

#endif
