/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: eeprom_24lc64.h
 ****************************************************************************/

#ifndef __EEPROM_24LC64_H__
#define __EEPROM_24LC64_H__

#define I2C_ADDR_24LC64_R100	0xA0
#define I2C_ADDR_24LC64_R200	0xAC
#define MAX_PAGE_SIZE		32
#define ADDRESS_SIZE		2

#define MAX_EEPROM_SIZE 0x4000

/*****************************************************************************
 * FUNCTION:
 * 	init_eeprom
 *
 * PURPOSE:
 * 	To initialize the EEPROM
 *
 * RETURNS:
 * 	1 = Success
 * 	0 = Error
 ****************************************************************************/
int init_eeprom (void);

/*****************************************************************************
 * FUNCTION:
 * 	write_data_eeprom
 *
 * PURPOSE:
 * 	To Write Data to the EEPROM
 *
 * IN:
 * 	u16 address - The internal Address to write to in the EEPROM
 *
 * 	u8 *data - This is the data that we are writing
 *
 * 	u16 *size - This is the amount of data we want to write.
 *
 * OUT:
 * 	u16 *size - This is the amount of data that we wrote.
 * 	
 * RETURNS:
 * 	1 = Success
 * 	0 = Error
 ****************************************************************************/
int write_data_eeprom (u16 address, u8 const *data, u16 *size);

/*****************************************************************************
 * FUNCTION:
 * 	read_data_eeprom
 *
 * PURPOSE:
 * 	To Read Data from the EEPROM
 *
 * IN:
 * 	u16 address - the internal Address to read from the EEPROM
 *
 * 	u16 *size - This is the amount of data we want to read
 *
 * OUT:
 * 	u8 *data - This is the Data that was read
 * 	u16 *size - This is the number of bytes that was read
 * 	
 * RETURNS:
 * 	1 = Success
 * 	0 = Error
 ****************************************************************************/
int read_data_eeprom (u16 address, u8 *data, u16 *size);

#endif // __EEPROM_24LC64_H__
