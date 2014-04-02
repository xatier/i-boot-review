/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: i2c_boot.h
 *
 * PURPOSE:
 * 	This is the generic I2C header file.  If you are a device and want
 * 	to use I2C then you should be including this file only.
 ****************************************************************************/

#ifndef __I2C_BOOT_H__
#define __I2C_BOOT_H__

/*****************************************************************************
 * FUNCTION:
 * 	init_i2c
 *
 * PURPOSE:
 * 	To do all of the initialization of the I2C interface.   That is why
 * 	this function does not take any values.
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
int init_i2c (void);

/*****************************************************************************
 * FUNCTION:
 * 	write_data_i2c
 *
 * PURPOSE:
 * 	To write data out to the I2C device.
 *
 * IN:
 * 	unsigned char slave_address - This is the I2C address of the device.
 *
 * 	unsigned char *data - This is the data to be written to the I2C device
 *
 * IN / OUT:
 * 	unsigned short *size - This is the amount of data when passed to this
 * 		function and holds the number of bytes that were actually
 * 		written when the function returns.
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 *
 * SPECIAL NOTES:
 * 	This function requires that the I2C driver for the I2C device does
 * 	not send this function more data that the device is able to handle.
 * 	This is mostly only important because we do page based writes and
 * 	hardcoding it into this driver may break other things later.
 ****************************************************************************/
int write_data_i2c (unsigned char slave_address, unsigned char *data,
	       unsigned short *size);

/*****************************************************************************
 * FUNCTION:
 * 	read_data_i2c
 *
 * PURPOSE:
 * 	To read data from the I2C device.
 *
 * IN:
 * 	unsigned char slave_address - This is the I2C address of the device
 *
 * 	unsigned char *address - This is the address to be used inside of the
 * 		I2C device
 *
 * OUT:
 * 	unsigned char *data - This is where the data to be read will be put
 *
 * IN / OUT:
 * 	unsigned short *size - This is the amount of data to be read on input
 * 		and the amount of data read on output
 * 	
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 * 
 * SPECIAL NOTES:
 * 	This function requires that the I2C driver for the I2C device does
 * 	not send this function more data that the device is able to handle.
 * 	This is mostly only important because we do page based writes and
 * 	hardcoding it into this driver may break other things later.
 ****************************************************************************/
int read_data_i2c (unsigned char slave_address, unsigned char *address, 
		unsigned char *data, unsigned short *size);

#endif // __I2C_BOOT_H__

