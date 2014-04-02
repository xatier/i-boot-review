//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      platform.c
//
// Description:
//
//      Functions that return information about the platform being run, such as
//      RAM/Flash size/location/range.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      December 2001
//
////////////////////////////////////////////////////////////////////////////////
#include <types.h>
#include <c_main.h>
#include <platform.h>

////////////////////////////////////////////////////////////////////////////////
// flash_block_size_platform
// PURPOSE: Returns the block size for the installed flash chip.
// PARAMS:  None. (Uses global variable status)
// RETURNS: u32 - size of flash blocks
////////////////////////////////////////////////////////////////////////////////
u32
flash_block_size_platform(void)
{
   switch(status.flash_chip)
   {
      case i128:
      {
	 return 0x20000;
	 break;
      }
      case i128x2:
      {
	 return 0x40000;
	 break;
      }
      default:
      {
	 return 0;
	 break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// flash_base_platform
// PURPOSE: Returns the base address of flash.
// PARAMS:  None. (Uses global variable status)
// RETURNS: u32 - flash starting address.
////////////////////////////////////////////////////////////////////////////////
u32
flash_base_platform(void)
{
   return 0x04000000;//return 0;
}

////////////////////////////////////////////////////////////////////////////////
// flash_size_platform
// PURPOSE: Returns the total size of flash, in bytes.
// PARAMS:  None. (Uses global variable status)
// RETURNS: u32 - flash size.
////////////////////////////////////////////////////////////////////////////////
u32
flash_size_platform(void)
{
   switch(status.flash_chip)
   {
      case i128:
      {
         return 16*1024*1024;
         break;
      }
      case i128x2:
      {
         return 32*1024*1024;
         break;
      }
      default:
      {
         return 0;
         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// mem_size_platform
// PURPOSE: Returns the size of RAM, in bytes
// PARAMS:  None. (Uses global variable status)
// RETURNS: u32 - RAM size.
////////////////////////////////////////////////////////////////////////////////
u32
mem_size_platform(void)
{
   return status.memsize;
}

////////////////////////////////////////////////////////////////////////////////
// mem_base_platform
// PURPOSE: Returns the base address of RAM.
// PARAMS:  None. (Uses global variable status)
// RETURNS: u32 - base address of RAM
////////////////////////////////////////////////////////////////////////////////
u32
mem_base_platform(void)
{
   switch(status.processor)
   {
      case sa1110:
      {
	 return 0xc0000000;
	 break;
      }
      case xscale:
      {
	 return 0xa0000000;
	 break;
      }
      default:
      {
	 //there's not really any good default for this
	 return 0xFFFFFFFF;
	 break;
      }
   }
}
