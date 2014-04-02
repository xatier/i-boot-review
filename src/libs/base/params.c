//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      params.c
//
// Description:
//
//      Interfaces with the Linux kernel command string functions.
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

#include <types.h>
#include <params.h>
#include <asm/setup.h>
//these may not be in the above, depending on kernel version.
#ifndef tag_next
#define tag_next(t)   ((struct tag *)((u32 *)(t) + (t)->hdr.size))
#endif
#ifndef tag_size
#define tag_size(type)  ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)
#endif

////////////////////////////////////////////////////////////////////////////////
// kernel_param
// PURPOSE: Sets up a Linux kernel command string in memory.
// PARAMS:  (IN) char *commandline - kernel command string.
// RETURNS: Address at which parameter list is stored.
////////////////////////////////////////////////////////////////////////////////
struct tag *
kernel_param(char const *commandline)
{
   struct tag *params = (struct tag *)BOOT_PARAMS;
   int i;

   params->hdr.size = tag_size(tag_core);
   params->hdr.tag = ATAG_CORE;
   params->u.core.flags = 0;
   params->u.core.pagesize = 4096;//0;
   params->u.core.rootdev = ((1<<8)|0);//0;
   params = tag_next(params);

   // Cerf kernels set up memory banks themselves and don't react well when
   // we set up an ATAG_MEM as well.
#if 1	//#ifdef SET_ATAG_MEM
   params->hdr.size = tag_size(tag_mem32);
   params->hdr.tag = ATAG_MEM;
   params->u.mem.size = PLATFORM_MEMORY_SIZE;
   params->u.mem.start = PLATFORM_MEMORY_BASE;
   params = tag_next(params);
#endif //SET_ATAG_MEM

#if 1	//#ifdef SET_ATAG_RAMDISK
   params->hdr.size = tag_size(tag_ramdisk);
   params->hdr.tag = ATAG_RAMDISK;
   params->u.ramdisk.flags = 0;
   params->u.ramdisk.size = 16384;
   params->u.ramdisk.start = 0;
   params = tag_next(params);
#endif

#if 1	//#ifdef SET_ATAG_INITRD
   params->hdr.size = tag_size(tag_initrd);
   params->hdr.tag = ATAG_INITRD;
   params->u.initrd.start = 0xC1000000; // MMU-mapped address
   params->u.initrd.size = 8*1024*1024;
   params = tag_next(params);
#endif //SET_ATAG_INITRD

   if (commandline && *commandline)
   {
       params->hdr.tag = ATAG_CMDLINE;
       for (i=0; commandline[i] != 0; ++i)
          params->u.cmdline.cmdline[i] = commandline[i];
       params->u.cmdline.cmdline[i] = 0;
       params->hdr.size = tag_size(tag_cmdline) + ((i+3) / 4);
       params = tag_next(params);
   }

   // List is terminated by a zero length tag
   params->hdr.size = 0;
   params->hdr.tag = ATAG_NONE;

   return (struct tag *)BOOT_PARAMS;
}
