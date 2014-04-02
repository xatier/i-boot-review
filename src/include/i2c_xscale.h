/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: i2c_xscale.h
 *
 * PURPOSE:
 * 	This file is the header file for the I2C work for the XScale
 * 	processor.  This file should not be included by other source
 * 	files, i2c.h should be included by other source files.
 ****************************************************************************/

#ifndef __I2C_XSCALE_H__
#define __I2C_XSCALE_H__

/*
 * Here are the Addresses of Registers that we will have to use
 */
#define _IBMR		0x40301680	// I2C Bus Monitor Register
#define _IDBR		0x40301688	// I2C Data Buffer Register
#define _ICR 		0x40301690	// I2C Control Register
#define _ISR 		0x40301698	// I2C Status Register
#define _ISAR 		0x403016A0	// I2C Slave Address Register

#define _CKEN		0x41300004	// Clock Enable Register

/*
 * This macro should make X so we can do
 *
 * __REG(0x10) = 0x20;  this means that address 0x10 should get data 0x20
 * crude example but it shows how it works.  It should do a 32 bit read /
 * write.
 */
#define __REG(x) 	(*((volatile unsigned int *)(x)))
#define __REG8(x) 	(*((volatile unsigned char *)(x)))
#define __REG16(x) 	(*((volatile unsigned short *)(x)))
#define __REG32(x) 	__REG(x)

/*
 * These macros should enable a person to do individual reads and writes
 * in 1,2 and 4 byte sizes
 */
#define WRITE_REG8(addr,data) 	__REG8(addr) = (data)
#define READ_REG8(addr)		__REG8(addr)
#define WRITE_REG16(addr,data) 	__REG16(addr) = (data)
#define READ_REG16(addr)	__REG16(addr)
#define WRITE_REG(addr,data) 	__REG(addr) = (data)
#define READ_REG(addr)		__REG(addr)
#define WRITE_REG32(addr,data) 	WRITE_REG(addr,data)
#define READ_REG32(addr)	WRITE_REG(addr)

/*
 * There are some macros that should allow us to basically do
 * IBMR = value for example or the other way around
 */
#define IBMR		__REG(_IBMR)	// I2C Bus Monitor Register
#define IDBR		__REG(_IDBR)	// I2C Data Buffer Register
#define ICR		__REG(_ICR)	// I2C Control Register
#define ISR		__REG(_ISR)	// I2C Status Register
#define ISAR		__REG(_ISAR)	// I2C Slave Address Register

#define CKEN		__REG(_CKEN)	

#define CKEN14_I2C	(1<<14)		// I2C unit clock enable
/*
 * These are definitions for the individual bits in the Various I2C
 * Registers
 */
#define I2C_ICR_FM	(1<<15)		// ICR - Fast Mode
#define I2C_ICR_UR	(1<<14)		// ICR - Unit Reset
#define I2C_ICR_ALDIE	(1<<12)		// ICR - Arbitration Loss Detected 
					// 	 IRQ Enable
#define I2C_ICR_MA	(1<<10)		// ICR - Master abort
#define I2C_ICR_GCD	(1<<7)		// ICR - I2C General Call Disable
#define I2C_ICR_IUE	(1<<6)		// ICR - I2C Unit Enable
#define I2C_ICR_SCLE	(1<<5)		// ICR - SCL Enable
#define I2C_ICR_TB	(1<<3)		// ICR - Transmit Byte
#define I2C_ICR_ACKNAK	(1<<2)		// ICR - ACK / NAK
#define I2C_ICR_STOP	(1<<1)		// ICR - Start
#define I2C_ICR_START	(1<<0)		// ICR - Stop

/*
 * I2C Interrupt Status Register Bit Defines
 */
#define I2C_ISR_BED	0x0400	// Bus Error Detected
#define I2C_ISR_SAD	0x0200	// Slave Address Detected
#define I2C_ISR_GCAD	0x0100	// General Call Address Detected
#define I2C_ISR_IRF	0x0080	// IDBR Receive Full
#define I2C_ISR_ITE	0x0040	// IDBR Transmit Empty
#define I2C_ISR_ALD	0x0020	// Arbitration Loss Detected
#define I2C_ISR_SSD	0x0010	// Slave Stop Detected
#define I2C_ISR_IBB	0x0008	// I2C Busy Busy
#define I2C_ISR_UB	0x0004	// Unit Busy
#define I2C_ISR_ACKNAK	0x0002	// ACK/NAK Status
#define I2C_ISR_RWM	0x0001	// Read / Write Mode
#define I2C_ISR_RESET_BITS	0x06F0 // BED | SAD | IRF | ITE | ALD | SSD

/*
 * Here are some Macros made to make reading the code easier.
 */
#define SETUP_START 	ICR |= I2C_ICR_START
#define SETUP_STOP	ICR |= I2C_ICR_STOP
#define END_START	ICR &= ~I2C_ICR_START
#define END_STOP	ICR &= ~I2C_ICR_STOP
#define TRANSMIT_DATA	ICR |= I2C_ICR_TB
#define RECEIVE_DATA	ICR |= I2C_ICR_TB
#define SET_ACK		ICR &= ~I2C_ICR_ACKNAK
#define SET_NAK		ICR |= I2C_ICR_ACKNAK
#define DISABLE_ALDIE	ICR &= ~I2C_ICR_ALDIE

static inline void idbr_tx_done( void)
{
        int timeout = 1000;
        while (timeout && ((ISR & I2C_ISR_ITE) == 0))
        {
                udelay(10);
                timeout--;
        }
        ISR = I2C_ISR_ITE;
}
#define IDBR_TX_DONE idbr_tx_done()

static inline void idbr_rx_done( void)
{
        int timeout = 1000;
        while (timeout && ((ISR & I2C_ISR_IRF) == 0))
        {
                udelay(10);
                timeout--;
        }
        ISR = I2C_ISR_IRF;
}
#define IDBR_RX_DONE idbr_rx_done()

static inline void i2c_bus_ready( void)
{
        int timeout = 1000;
        while (timeout && (ISR & I2C_ISR_IBB))
        {
                udelay(10);
                timeout--;
        }
}
#define I2C_BUS_READY i2c_bus_ready()

/*
 * These Macros will generate the Control Byte, which is Slave Address
 * and a R/W byte
 */
#define I2C_CONTROL_BYTE_READ(addr,slave) (addr)=((slave) | 1)
#define I2C_CONTROL_BYTE_WRITE(addr,slave) (addr)=((slave) & ~1)

#endif // __I2C_XSCALE_H__
