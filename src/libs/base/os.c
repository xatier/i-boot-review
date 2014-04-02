//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      os.c
//
// Description:
//
//      Handles the loading/booting of OS images.
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

#include <c_main.h>
#include <flash_i128.h>
#include <util.h>
#include <parser.h>
#include <string.h>
#include <cedecode.h>
#include <platform.h>
#include <os.h>
#include <messages.h>
#include <params.h>

void inc_led(void);

////////////////////////////////////////////////////////////////////////////////
// detect_linux_kernel
// PURPOSE: Look for a Linux kernel in flash
// PARAMS:  (IN) char *kernel - purported location of kernel
// RETURNS: 1 if found, 0 if not
////////////////////////////////////////////////////////////////////////////////
static inline int
detect_linux_kernel(char const *kernel)
{
   return *((u32 *)kernel + 9) == LINUX_SIG;
}

////////////////////////////////////////////////////////////////////////////////
// boot_linux_kernel
// PURPOSE: Boot the linux kernel
// PARAMS:  (IN) char *arg - boot parameter string
//          (IN) u8 *addr - kernel start address
// RETURNS: Nothing.
// NOTES:   
////////////////////////////////////////////////////////////////////////////////
void 
boot_linux_kernel( char const *arg, u8 *addr)
{
   char param[MAX_PARAM_SIZE];
   u8 *ram_start = addr;
   struct tag *tagparams;
   void (*linux_kernel)(int zero, int arch, struct tag *params);
   u32 zImage_id;

#ifdef DEBUG
   itc_printf("ARG=0x%x ADDR=0x%x\r\n", arg, addr);
#endif

   //are there any kernel parameters?
   param[0] = '\x0';
   if (arg && *arg!='\0')
   {
      if (kernel_param_parse(arg, param, sizeof(param)) < 0)
      {
          // Error parsing parameters
          return;
      }
   }
   // Set up tagged parameter structure in memory
   tagparams = kernel_param(param);

   if( ram_start == (u8*)0) 
   {
      //grab zImage from flash
      if( (status.processor == xscale) ||
          (status.processor == sa1110))
      {
         //zImage knows where it wants to go
	     ram_start = *(u8 **)LINKADDR_START_FLASH;
	     if( ram_start == (u8*)0)
	     {
	        ram_start = (u8*)KERNEL_RAM_START;
	     }
      }
      else
      {
         itc_printf("os.c: unknown processor\r\n");
      }

      if (!detect_linux_kernel((char *)KERNEL_FLASH_START))
      {
         itc_printf("ERROR: No Linux kernel detected at 0x%x\r\n",
                    KERNEL_FLASH_START);
         return;
      }

      // copy zImage from flash to ram
      // Newer kernels can actually run directly out of Flash
      itc_printf("Relocating zImage from %x to %x (len=%x)\r\n",
                 KERNEL_FLASH_START, ram_start, KERNEL_MAX_SIZE);
      itc_memcpy(ram_start, (u8 *)KERNEL_FLASH_START, KERNEL_MAX_SIZE);
#if 1
      itc_printf("Relocating ramdisk.gz from %x to %x (len=%x)\r\n",
                 RAMDISK_FLASH_START, RAMDISK_RAM_START, RAMDISK_MAX_SIZE);
      itc_memcpy((u8 *)RAMDISK_RAM_START, (u8 *)RAMDISK_FLASH_START, RAMDISK_MAX_SIZE);
#endif
   }
   else
   {
      //zImage is in memory
      u8 *zImage_ram_start = *(u8 **)(ram_start+LINKADDR_START_OFFSET);

      if (!detect_linux_kernel(ram_start))
      {
         itc_printf("ERROR: No Linux kernel detected at 0x%x\r\n",
                    ram_start);
         return;
      }

      if( (zImage_ram_start != ram_start) && (zImage_ram_start!=(u8*)0))
      {
         //need to relocate zImage to zImage_ram_start
         u32 zImage_size =  *(u8 **)(ram_start+LINKADDR_END_OFFSET) - zImage_ram_start;

         itc_printf("Relocating zImage from %x to %x (len=%x)\r\n",
            ram_start, zImage_ram_start, zImage_size);
         itc_memmove(zImage_ram_start, ram_start, zImage_size);

         ram_start = zImage_ram_start;
      }
   }

   zImage_id = *(u32 *)(ram_start+ARM_ZIMAGE_ID_OFFSET);
   if( zImage_id != ARM_ZIMAGE_ID)
   {
      itc_printf("os.c: zImage does not have proper signature!\r\n");
   }
   else
   {
      itc_printf("Proper ARM zImage ID found. Booting...\r\n");
   }

#ifdef DEBUG
   itc_printf("ID=%x zstart=%x zend=%x\r\n", 
      *(u32 *)(ram_start+ARM_ZIMAGE_ID_OFFSET),
      *(u32 *)(ram_start+LINKADDR_START_OFFSET),
      *(u32 *)(ram_start+LINKADDR_END_OFFSET));

//   inc_led causes problems on xscale even though its a no-op !?!
//   inc_led();

   itc_printf("RAM=0x%x FLASH=0x%x\r\n", ram_start, KERNEL_FLASH_START);
#endif

   linux_kernel = (void (*)(int, int, struct tag *))ram_start;
   linux_kernel(LINUX_ARG0, LINUX_ARG1, tagparams);
}

////////////////////////////////////////////////////////////////////////////////
// init_os
// PURPOSE: A wrapper to call run_os with no arguments. This is used when doing
//          the initial boot (after the timeout).
// PARAMS:  None.
// RETURNS: Nothing.
// NOTES:   If unable to boot the default os, init_os will return, allowing the
//          user interface to take over (so the user can fix whatever the
//          problem is). Normally, this function will not return.
////////////////////////////////////////////////////////////////////////////////

void
init_os(void)
{
   run_os( NO_BOOT_PARAMS );
}

////////////////////////////////////////////////////////////////////////////////
// run_os
// PURPOSE: Brings the default os up to a bootable state, then boots it.
// PARAMS:  (IN) char* arg - the Linux kernel command line arguments. Ignored 
//               for CE boot.
// RETURNS: Nothing.
// NOTES:   If unable to boot the default os, run_os will return, allowing the
//          user interface to take over (so the user can fix whatever the
//          problem is). Normally, this function will not return.
////////////////////////////////////////////////////////////////////////////////

void
run_os(char const * arg)
{
   if (detect_wince_kernel((char *)KERNEL_FLASH_START))
   {
      void (*ce_kernel)(void);

      // Clear the first 2 MB of RAM to make sure old remnants of WinCE are
      // gone so it will reliably boot
      message_print(RAM_CLEAR_MESSAGE);
      memset32((u32 *)PLATFORM_MEMORY_BASE, 0, CE_CLEAR_SIZE / sizeof(u32));

      message_print(LOADING_CE_MESSAGE);
      inc_led();

      ce_kernel = (void (*)(void))(cedecode((u8 *)CE_RAM_BASE,
                                            (u8 *)KERNEL_FLASH_START));

      ce_kernel();
   }
   else if (detect_linux_kernel((char *)KERNEL_FLASH_START))
   {
      boot_linux_kernel( arg , KERNEL_USE_DEFAULT_ADDR);
   }
   else
   {
      error_print(NO_OS_ERROR);
   }
}
