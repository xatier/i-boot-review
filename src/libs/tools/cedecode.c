//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      cedecode.c
//
// Description:
//      Decodes a Windows CE .bin file into memory, and returns the location
//      of the CE kernel.
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

#include <config.h>
#include <cedecode.h>
#include <c_main.h>
#include <util.h>
#include <string.h>
#include <platform.h>
#include <debug.h>

////////////////////////////////////////////////////////////////////////////////
// detect_wince_kernel
// PURPOSE: Look for a WinCE kernel in flash
// PARAMS:  (IN) char *kernel - purported location of kernel
// RETURNS: 1 if found, 0 if not
////////////////////////////////////////////////////////////////////////////////
int
detect_wince_kernel(char const *kernel)
{
   return (*(u32 *)kernel == CE_SIG_WORD1) &&
      ((*(u32 *)(kernel + sizeof(u32)) & 0x00FFFFFF) == CE_SIG_WORD2);
}

////////////////////////////////////////////////////////////////////////////////
// cedecode
// PURPOSE: Parses an MS BIN file
// PARAMS:  (IN) u8 *dest   - Memory location to parse into (currently unused).
//          (IN) u8 *source - Address of the MS BIN file.
// RETURNS: u32 - 0 on failure, address of the WinCE kernel on success.
////////////////////////////////////////////////////////////////////////////////
u32 *cedecode(u8 *dest, u8 const *source)
{
   u32 imagesize = 0;
   int vaddr = -1;
   int recordsize = -1;
   int basevaddr = 0;
   u32 *kerneladdr = 0;

   if (detect_wince_kernel(source))
   {
      DEBUG_5("Proper BIN file\r\n");
      source += CE_SIG_SIZE;
   }
   else
   {
      itc_printf("ERROR: Improper BIN file; no WinCE image detected\r\n");
      return NULL;
   }

   basevaddr = GETWORD(source);
   DEBUG_2("Image virtual address reported as: 0x%x\r\n", basevaddr);
   source += sizeof(u32);

   imagesize = GETWORD(source);
   DEBUG_2("Image size reported as: %i\r\n", imagesize);
   source += sizeof(u32);

   DEBUG_2("Loading\r\n");
   do
   {
      vaddr = GETWORD(source);
      source += sizeof(u32);
      recordsize = GETWORD(source);
      source += sizeof(u32);

      //we want to skip the record checksum as well.
      source += sizeof(u32);

      DEBUG_5("Record at vaddr 0x%x of size %i.\r\n", vaddr, recordsize);

      if(vaddr != 0)
         itc_memcpy((u8 *)((vaddr - basevaddr) + CE_RAM_BASE),
                  (u8 *)source, recordsize);
      else
      {
         kerneladdr = (u32 *)((recordsize - basevaddr) + CE_RAM_BASE);
         break;
      }
      source += recordsize;
   } while(1);
     //XXX: this should die if the checksum is wrong, but we don't check it yet.

   DEBUG_TRACE("\rKernel address reported: 0x%x\r\n", (u32)kerneladdr);
   return kerneladdr;
}
