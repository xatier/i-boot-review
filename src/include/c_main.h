//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      c_main.h
//
// Description:
//
//      Initializes anything that needs to be initialized, and starts the UI or
//      default OS.
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

#ifndef C_MAIN_H
#define C_MAIN_H

#include <types.h>
#include <config.h>
#include <dhcp.h>

#define CE_RAM_BASE             (mem_base_platform() + 0x200000)

#define MAX_SCRIPT_SIZE 1024

#define LINUX_ARG0 (0)

// Kernel machine types
#ifdef XSCALE
#define LINUX_ARG1 (139)        // PXA250 Cerf
#else
#define LINUX_ARG1 (31)         // SA1110 Cerf
#endif

#define BL_TEMP_RAM             (mem_base_platform() + 0xA00000)
#define CE_TEMP_RAM             (mem_base_platform() + 0x9FFFF9)

#define KERNEL_FLASH_START      (PLATFORM_FLASH_BASE+(MAIN_BLOCK_SIZE*3))	//(MAIN_BLOCK_SIZE * 3)
#define ARM_ZIMAGE_ID		0x016f2818
#define ARM_ZIMAGE_ID_OFFSET	0x24
#define LINKADDR_START_OFFSET	0x28
#define LINKADDR_END_OFFSET	0x2c
#define ARM_ZIMAGE_ID_FLASH	(KERNEL_FLASH_START+ARM_ZIMAGE_ID_OFFSET)
#define LINKADDR_START_FLASH	(KERNEL_FLASH_START+LINKADDR_START_OFFSET)
#define LINKADDR_END_FLASH	(KERNEL_FLASH_START+LINKADDR_END_OFFSET)

//
// The ram start has to match the ZTEXTADDR which is defined in the
// the Linux kernel in arch/arm/boot/Makefile.
//
#define KERNEL_RAM_START	(mem_base_platform() + 0x8000)
#define KERNEL_USE_DEFAULT_ADDR	0

#define KERNEL_MAX_SIZE         (0x100000)

#define RAMDISK_FLASH_START (PLATFORM_FLASH_BASE+0x001C0000)
#define RAMDISK_RAM_START	(mem_base_platform() + 0x01000000)
#define RAMDISK_MAX_SIZE		(0x800000)

typedef enum
{
   Linux = 0,
   CE = 1
} os_type;

typedef enum
{
   dl_tftp,
   dl_tftpd,
   dl_serial
} dl_method;

typedef enum
{
   i128,
   i128x2
} flash_type;

typedef enum
{
   sa1110,
   xscale
} processor_type;

typedef struct {
   dl_method dl;
   os_type os;
   u32 *CEKernel;
   u32 ciaddr;
   u32 siaddr;
   u32 giaddr;
   u32 smask;
   char bootfile[MAX_BOOTFILE_SIZE];
   u16 macaddr[3];
   flash_type flash_chip;
   u32 memsize;
   processor_type processor;
} iboot_status;

extern iboot_status status;
extern const char copyright[];

char kname[15];

#endif //C_MAIN_H
