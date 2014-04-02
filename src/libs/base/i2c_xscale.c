/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: i2c_xscale.c
 *
 * PURPOSE:
 * 	This file is the XScale Driver for the I2C Controller on the XScale
 * 	its purpose is to allow the usage of more than one I2C device, and
 * 	to also make it easier to develop bootloader drivers, for I2C devices.
 ****************************************************************************/
#include <types.h>
#include <timer.h>
#include <i2c_xscale.h>
#include <i2c_boot.h>

int init_i2c (void)
{
	/*
	 * Here we set up the I2C controller for generic defaults.
	 * If you device isn't working, and you think it might be
	 * the I2C setup, check here
	 */

	/*
	 * I2C Unit Reset
	 */
	ICR = I2C_ICR_UR;
	
	/* Enable the I2C clock */
	CKEN |= CKEN14_I2C;
	
	ICR = I2C_ICR_GCD | I2C_ICR_SCLE; 

	ISR = I2C_ISR_RESET_BITS;

	I2C_BUS_READY;

	ICR |= I2C_ICR_IUE;

	return 1;
} // End of init_i2c


int write_data_i2c (u8 slave_address, u8 *data, u16 *size)
{
	unsigned char addr=0;
	unsigned short nr_copied=0;
	int i=0;

	I2C_BUS_READY;
	
	/*
	 * Setup and Send Control Byte
	 */
	I2C_CONTROL_BYTE_WRITE (addr, slave_address);
	IDBR = addr;

	SETUP_START;
	END_STOP;
	DISABLE_ALDIE;
	TRANSMIT_DATA;

	/*
	 * Transmit all Data until we are done
	 */	
	for (i=0; i<*size; i++,nr_copied++)
	{
	
		IDBR_TX_DONE;
		
		END_START;
		if (i == (*size - 1))
			SETUP_STOP;
		else
			END_STOP;

		/*
		 * Write Data to I2C Data Buffer Register
		 */
		IDBR = data[i];
		TRANSMIT_DATA;
	}
	
	*size = nr_copied;
	IDBR_TX_DONE;

        if( ISR & I2C_ISR_BED)
        {
                ISR = I2C_ISR_BED;
                return 0;
        }

	END_STOP;
	
	return 1;
	
} // End of write_data_i2c


int read_data_i2c (u8 slave_address, u8 *address, u8 *data, u16 *size)
{
	unsigned char addr=0;
	unsigned short nr_copied=0;
	int i=0;

	if (*size == 0)
		return 1;
	/*
	 * Start the Setting of the Address by writing the Control
	 * Byte and Address (3 writes)
	 */
	I2C_CONTROL_BYTE_WRITE(addr, slave_address);
	IDBR = addr;

	SETUP_START;
	END_STOP;
	DISABLE_ALDIE;

	TRANSMIT_DATA;
	IDBR_TX_DONE;
	
	IDBR = address[0];
	END_START;
	END_STOP;
	
	TRANSMIT_DATA;
	IDBR_TX_DONE;

	IDBR = address[1];
	END_START;
	END_STOP;

	TRANSMIT_DATA;
	IDBR_TX_DONE;
	
	/*
	 * Start bytes Read, by writing the Control Byte
	 */
	I2C_CONTROL_BYTE_READ(addr,slave_address);
	IDBR = addr;

	SETUP_START;
	END_STOP;
	TRANSMIT_DATA;

	IDBR_TX_DONE;

	/* Read */
	for (i=0; i<*size; i++,nr_copied++)
	{
		END_START;
		if ( i == (*size-1) )
		{
			SET_NAK;
			SETUP_STOP;
		}
		else
		{
			SET_ACK;
			END_STOP;
		}
	
		RECEIVE_DATA;
		IDBR_RX_DONE;
		data[i] = IDBR;
	}

	*size = nr_copied;
	END_STOP;
	SET_ACK;

	return 1;
	
} // End of read_data_i2c

