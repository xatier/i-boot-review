//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      flash_i128.c
//
// Description:
//
//      Driver for Intel StrataFlash chips.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      October 2001
//
////////////////////////////////////////////////////////////////////////////////

#include <flash_i128.h>
#include <string.h>
#include <c_main.h>
#include <platform.h>
#include <messages.h>
#include <debug.h>

////////////////////////////////////////////////////////////////////////////////
// status_flash
// PURPOSE: Checks the flash status for error.
// PARAMS   (IN) volatile u16 *flash_addr - flash address to check for errors.
// RETURNS: 1 on error, 0 otherwise.
////////////////////////////////////////////////////////////////////////////////
static int
status_flash(volatile u32 *flash_addr)
{
   u32 flash_status = 0;
   int error = 0;

   if(!wait_flash(flash_addr))
   {
      return 1;
   }

   //make sure status is up to date
   {
      volatile u32 foo;
      foo = DEREF(flash_addr);
      foo = DEREF(flash_addr);
      foo = DEREF(flash_addr);
   }

   DEREF(flash_addr) = STATUS_READ;

   flash_status = DEREF(flash_addr);
   if(flash_status & STATUS_PGM_ERR)
   {
      error_print(FLASH_PROGRAMMING_ERROR);
      error = 1;
   }
   if(flash_status & STATUS_PROTECT)
   {
      error_print(FLASH_PROTECTED_ERROR);
      error = 1;
   }
   if(flash_status & STATUS_VOLTAGE_ERR)
   {
      error_print(FLASH_PROGRAMMING_ERROR);
      error = 1;
   }
   if(flash_status & STATUS_ERASE_ERR)
   {
      error_print(FLASH_ERASE_ERROR);
      error = 1;
   }

   if(error)
   {
      itc_printf("flash_status: 0x%x\r\n", flash_status);
   }

   return error;
}

////////////////////////////////////////////////////////////////////////////////
// init_flash
// PURPOSE: This is the initialization function for the flash driver. Here is
//          where things such as buswidth should be set and so on.
// PARAMS:  None.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
init_flash(void)
{
  //This chip needs no initialization.
} // end of init_flash

////////////////////////////////////////////////////////////////////////////////
// block_erase_flash
// PURPOSE: To erase an entire block of flash.
// PARAMS:  (IN) u32 uBlockAddr - This is the starting address of the block that
//                                 we wish to erase.
// RETURNS  1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
block_erase_flash(u32 *addr)
{
   volatile u32 *flash = addr;
   int error = 0;

   if(!wait_flash((u32 *)flash))
   {
      return 0;
   }

   DEREF(flash) = CAST(STATUS_CLEAR);

   if(!wait_flash(flash))
   {
      return 0;
   }

   DEREF(flash) = CAST(CMD_CLEAR_LOCK);
   DEREF(flash) = CAST(CMD_CONFIRM);

   if(!wait_flash(flash))
   {
      return 0;
   }

   DEREF(flash) = CAST(ERASE_SETUP);
   DEREF(flash) = CAST(ERASE_CONFIRM);

   if(!wait_flash(flash))
   {
      return 0;
   }

   error = status_flash(flash);
   if(error)
   {
      DEBUG_0("bad erase: 0x%x\r\n", (u32)flash);
   }

   DEREF(flash) = CAST(READ_ARRAY);

   return !error;
} // end of block_erase_flash

////////////////////////////////////////////////////////////////////////////////
// block_write_flash_sector
// PURPOSE: Writes one sector of flash
// PARAMS:  (IN) u32 *dest - address to write to.
//          (IN) u32 *src  - data to write. Should have one full block.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
block_write_flash_sector(u32 *dest,
                         u32 *src)
{
   unsigned long i = 0;
   volatile u8 *flash = (u8 *)dest;
   u8 *data = (u8 *)src;
   int error = 0;
   int pollmax = POLLMAX;

   if(!block_erase_flash((u32 *)flash))
      return 0;

   while(i < MAIN_BLOCK_SIZE)
   {
      int cnt = WR_BUFF_SIZE / sizeof(u16);

      if(!wait_flash((u32 *)flash))
      {
	 return 0;
      }

      do
      {
         DEREF(flash) = CAST(CMD_WR_BUFF);
      } while( pollmax-- && (DEREF(flash) & CAST(STATUS_BUSY)) !=
		             CAST(STATUS_BUSY));

      if(!pollmax)
      {
         error_print(FLASH_DEAD_ERROR);
         return 0;
      }

      if(status.flash_chip == i128x2)
      {
	 DEREF(flash) = CAST((WR_BUFF_SIZE / sizeof(u16) - 1) |
		             ((WR_BUFF_SIZE / sizeof(u16) - 1) <<
			      (FLASH_BUS_WIDTH * 4)));
      }
      else if(status.flash_chip == i128)
      {
	 DEREF(flash) = CAST(WR_BUFF_SIZE / FLASH_BUS_WIDTH - 1);
      }

      while( cnt-- )
      {
         DEREF((flash + i)) = DEREF(data);
         i += FLASH_BUS_WIDTH;
	 data += FLASH_BUS_WIDTH;
      }
      DEREF((flash + i)) = CAST(CMD_CONFIRM);

      if(!wait_flash((u32 *)flash))
      {
	 return 0;
      }

      error = status_flash((u32 *)flash + i);
      if(error)
      {
         itc_printf("bad addr: %x\r\n", (u32)(flash + i));
         break;
      }
   }

   DEREF(dest) = CAST(READ_ARRAY);
   return error;
}

////////////////////////////////////////////////////////////////////////////////
// block_write_flash
// PURPOSE: To write one, or more, blocks of flash
// PARAMS:  (IN) u32 dest   - This is the starting address in flash to write
//                            the data to.
//          (IN) u32 source - This is the source address to read the data
//                            from.
//          (IN) u32 size   - This is the amount of data in bytes to write.
//          (IN) int perm   - Permissions. Certian blocks may be "off limits" to
//                            certian functions. For the moment, only the boot
//                            block is protected. To write to the boot block,
//                            parameter must be "FLASH_BOOT".
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
block_write_flash(u32 *dest,
                  u32 *source,
                  int size,
                  int perm)
{
   int error = 0;

   if(perm != FLASH_BOOT)
   {
      if((u32)dest < MAIN_BLOCK_SIZE)
      {
         error_print(FLASH_FLASHLOADER_ERROR);
         return -1;
      }
   }

   if((u32)dest > FLASH_MAX){
      DEBUG_0("Error: destination address (0x%x) is out of range.\r\n", (u32)dest);
      return 0;
   }

   itc_printf("0x");

   do
   {
      itc_printf("%x", (u32)dest);
      error = block_write_flash_sector((u32 *)dest, (u32 *)source);
      if(error)
         break;
      dest += MAIN_BLOCK_SIZE / sizeof(dest);
      source += MAIN_BLOCK_SIZE / sizeof(source);
      itc_printf("\b\b\b\b\b\b\b\b");
   } while((size -= MAIN_BLOCK_SIZE) > 0);

   itc_printf("%x\r\n", (u32)dest);
   return (error ? 0 : 1);
} // end of block_write_flash



////////////////////////////////////////////////////////////////////////////////
// block_read_flash
// PURPOSE:
//   There are many cases where this would not be neccessary to implement, but
//   there are a few cases where this might become neccessary.  If so it is here
//   otherwise use of it is optional.  (should just be a memcpy then).
// IN:
//   u32 uDestAddr - This is the starting location to write the data from
//     flash.
//
//   u32 uSrcAddr - This is the starting location of flash to read from.
//
//   u32 uSize - This is the size of data (in bytes) to read.
// SIDE EFFECTS:
//   This will cause an area of memory starting at uDestAddr to be written with
//   uSize bytes of data from uSrcAddr.
////////////////////////////////////////////////////////////////////////////////
int
block_read_flash(u32 uDestAddr,
                 u32 uSrcAddr,
                 u32 uSize)
{
  //memcpy(uDestAddr, uSrcAddr, uSize);
  return 1;
} // end of block_read_flash



////////////////////////////////////////////////////////////////////////////////
// FUNCTION: get_device_info_flash
// PURPOSE:
//   We need to get information about the flash, like manufacturer type, and
//   size of flash and so on.
// IN:
//   u32 uBlockAddr - This is the Address of the flash device.
// OUT:
//   u32 *pManufacturerID - This is the Manufacturer ID.  NOTE that on
//     16 and 8 bit flash interfaces that there will have to be some special
//     mangling done.
//
//   u32 *pDeviceID - This is the Device ID.  NOTE that on 16 and 8 bit
//     flash interfaces that there will have to be some special mangling done.
//
//   u32 *pBlockSize - This is set to the Block Size of the Flash.
//
//   u32 *pDeviceSize - This is the actual size of flash.
// SIDE EFFECTS:
//   None.
// SPECIAL NOTES:
//   On systems with a 16 or 8 bit buswidth, there is going to have to be some
//   special handling with the pManufacturerID and pDeviceID fields as those 
//   would normally be 32-bit fields.
////////////////////////////////////////////////////////////////////////////////
int
get_device_info_flash(u32 uBlockAddr,
		     u32 *pManufacturerID,
		     u32 *pDeviceID,
		     u32 *pBlockSize,
		     u32 *pDeviceSize)
{
  return 0;
} // end of get_device_info_flash



////////////////////////////////////////////////////////////////////////////////
// FUNCTION: wait_flash
// PURPOSE:
//   To wait till the flash chip is done executing the last command (most
//   likely either a write or clear.)
// IN:
//   u32 uBlockAddr - This is the Address of flash that we will be waiting
//     for
// SPECIAL NOTES:
//   After writing certain commands to the Flash, a period of time must elapse
//   for the actual flash chip to finish the command internally.  Because of
//   this we must wait for it to finish.  That should be done here.
////////////////////////////////////////////////////////////////////////////////
int 
wait_flash(volatile u32 *flash)
{
   int pollmax = POLLMAX;
   u32 flash_status;

   do
   {
      DEREF(flash) = CAST(STATUS_READ);
      flash_status = DEREF(flash);
   } while(--pollmax && (flash_status & CAST(STATUS_BUSY)) !=
	                 CAST(STATUS_BUSY));

   if(pollmax)
   {
      return 1;
   }
   else
   {
      error_print(FLASH_DEAD_ERROR);
      return 0;
   }
} // end of wait_flash

