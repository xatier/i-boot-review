/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: eeprom_24lc64.c
 *
 * FUNCTION:
 * 	This is the EEPROM 24LC64 driver for the Bootloader.  It is the
 * 	EEPROM that is used for the MAC Address, and depends on the I2C
 * 	driver.
 *
 ****************************************************************************/

#include <types.h>
#include <timer.h>
#include <i2c_boot.h>
#include <i2c_xscale.h>
#include <eeprom_24lc64.h>
#include <eeprom.h>
#include <string.h>

static u16 i2c_addr_24lc64;
/* list of possible slave addresses for 24lc64 eeprom */
static const u16 addr_list[] = {I2C_ADDR_24LC64_R200, I2C_ADDR_24LC64_R100, 0};

int
init_eeprom (void)
{
	int i=0;
	/* try to detect the slave addr */
	while( addr_list[i])
	{
		unsigned char dummy[1];
		unsigned short size=0;

		/* probe with a zero-length write */
                if (write_data_i2c (addr_list[i], dummy, &size))
		{
			i2c_addr_24lc64 = addr_list[i];
			ICR |= I2C_ICR_MA;
			return 1;
		}
		i++;
	}
#ifdef DEBUG_EEPROM
	/* probe all slave addressed */
	for( i=2; i<0xff; i+=2)
	{
		unsigned char dummy[1];
		unsigned short size=0;

                if (write_data_i2c (i, dummy, &size))
		{
			itc_printf("Found slave @ 0x%x\r\n", i>>1);
			ICR |= I2C_ICR_MA;
		}
	}
#endif

	/* detection failed, hope it's a R200 rev board */
	i2c_addr_24lc64 = I2C_ADDR_24LC64_R200;
	itc_printf("24LC64 not detected! (using %x)\r\n", i2c_addr_24lc64);

	return 1;
} // End of init_eeprom

int
write_data_eeprom (u16 address, u8 const *data, u16 *size)
{
	unsigned char data_out[MAX_PAGE_SIZE + ADDRESS_SIZE];
	int i=0;
	unsigned short sub_chunk_size=0, sub_chunks=0, nr_copied=0, chunk=0;
	unsigned short pos_in_page=0;

	/* If the address is not aligned with a page boundary, and address+(*size) 
	 * cross the current page boundary, we should first write the remainder of
	 * the current page, then write next pages. the 24AA64 datasheet didn't say
	 * so, but the chip works in this way.
	 */

	pos_in_page = address % MAX_PAGE_SIZE;

	if ( ( pos_in_page != 0 ) && (*size > (MAX_PAGE_SIZE-pos_in_page)) )
	{
		/*
		 * Setup the internal addresses for the EEPROM
		 */
		data_out[0] = (unsigned char)(((address + nr_copied) >> 8) & 0xFF);
		data_out[1] = (unsigned char)((address + nr_copied) & 0xFF);
		
		/*
		 * Copied the data into the output portion
		 */

		sub_chunk_size = MAX_PAGE_SIZE - pos_in_page + ADDRESS_SIZE;
		for (i=0;i < (sub_chunk_size - ADDRESS_SIZE);i++)
		{
			data_out[i+2] = data[nr_copied + i] ;
	
		}

		if (write_data_i2c (i2c_addr_24lc64, data_out, 
					&sub_chunk_size) < 0)
		{
			return 0;
		}
	    	
		nr_copied += (MAX_PAGE_SIZE - pos_in_page);
		*size -= (MAX_PAGE_SIZE - pos_in_page);
		udelay(50000);
	}
	
	/*
	 * Setup the Number of Chunks, sizes and such.  Because the
	 * 24LC64 EEPROM has a maximum number of pages that can be
	 * written via Page write, we chop what we were given up into
	 * blocks if it will not fit.
	 */
	sub_chunks = (*size / MAX_PAGE_SIZE);

	if (sub_chunks)
		sub_chunk_size = MAX_PAGE_SIZE + ADDRESS_SIZE;
	else
		sub_chunk_size = *size + ADDRESS_SIZE;

	if ((*size % MAX_PAGE_SIZE) != 0)
		sub_chunks++;
	
	for (chunk=0; chunk < sub_chunks; chunk++)
	{

		if ((chunk == (sub_chunks - 1)) &&\
			(*size % MAX_PAGE_SIZE))
			sub_chunk_size = (*size % MAX_PAGE_SIZE) + ADDRESS_SIZE;

		/*
		 * Setup the internal addresses for the EEPROM
		 */
		data_out[0] = (unsigned char)(((address + nr_copied) >> 8) & 0xFF);
		data_out[1] = (unsigned char)((address + nr_copied) & 0xFF);
		
		/*
		 * Copied the data into the output portion
		 */
		for (i=0;i < (sub_chunk_size - ADDRESS_SIZE);i++)
		{
			data_out[i+2] = data[nr_copied + i] ;
	
		}

		if (write_data_i2c (i2c_addr_24lc64, data_out, 
					&sub_chunk_size) < 0)
		{
			return 0;
		}
		
		nr_copied += (sub_chunk_size - ADDRESS_SIZE);
		udelay(50000);
	}
	
	*size = nr_copied;
	return 1;

} // End of write_eeprom

int
read_data_eeprom (u16 address, u8 *data, u16 *size)
{
	unsigned char addr[2];

	/*
	 * Setup Address for transmission
	 */
	addr[0] = (unsigned char)((address >> 8) & 0xFF);
	addr[1] = (unsigned char)(address & 0xFF);

	if (read_data_i2c (i2c_addr_24lc64, addr, data, size) < 0)
		return 0;
	
	return 1;
} // End of read_eeprom
