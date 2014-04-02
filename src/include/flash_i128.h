//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      flash_i128.h
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

#ifndef FLASH_I128_H
#define FLASH_I128_H

#include <types.h>
#include <config.h>

#define FLASH_DEFAULT 0
#define FLASH_BOOT    1

#define READ_ARRAY                0x00FF00FF
#define ERASE_SETUP               0x00200020
#define ERASE_CONFIRM             0x00D000D0
#define CMD_CLEAR_LOCK            0x00600060
#define CMD_WRITE                 0x00400040
#define PGM_SETUP                 CMD_WRITE
#define STATUS_READ               0x00700070
#define STATUS_CLEAR              0x00500050
#define CMD_READ_QUERY            0x00980098
#define CMD_WR_BUFF               0x00e800e8
#define CMD_CONFIRM               0x00D000D0

#define STATUS_PROTECT            0x00020002
#define STATUS_VOLTAGE_ERR        0x00080008
#define STATUS_PGM_ERR            0x00100010
#define STATUS_ERASE_ERR          0x00200020
#define STATUS_BUSY               0x00800080

#define POLLMAX		          (1<<25)
#define WR_BUFF_SIZE              32//16
//#ifdef FLASH_16MB
//#define NUM_FLASH_BLOCKS	  (64)
//#else
#define NUM_FLASH_BLOCKS	  (128)
//#endif
#define FLASH_BLOCK_BASE	  ((u32 *) 0x040000)
#define FLASH_BLOCK_SIZE	  ((u32) 0x040000)

#define MAIN_BLOCK_SIZE (flash_block_size_platform())
#define MAIN_BLOCK_WORDS          (MAIN_BLOCK_SIZE / 2)
#define MAIN_BLOCK_DWORDS         (MAIN_BLOCK_WORDS / 2)
#define FLASH_MAX                 (PLATFORM_FLASH_BASE+(MAIN_BLOCK_SIZE * NUM_FLASH_BLOCKS))

#define	KERNEL_START		  0x00040000
#define CE_BIN_START		  KERNEL_START


#if 0
#define READ_ARRAY                0x00FF
#define ERASE_SETUP               0x0020
#define ERASE_CONFIRM             0x00D0
#define CMD_CLEAR_LOCK            0x0060
#define CMD_WRITE                 0x0040
#define PGM_SETUP                 CMD_WRITE
#define STATUS_READ               0x0070
#define STATUS_CLEAR              0x0050
#define STATUS_BUSY               0x0080
#define STATUS_ERASE_ERR          0x0020
#define STATUS_PGM_ERR            0x0010
#define STATUS_PROTECT            0x0002
#define STATUS_VOLTAGE_ERR        0x0008
#define CMD_READ_QUERY            0x0098
#define CMD_WR_BUFF               0x00e8
#define CMD_CONFIRM               0x00D0

#define POLLMAX                   (1<<25)
#define WR_BUFF_SIZE              16
#define NUM_FLASH_BLOCKS          (128)
#define FLASH_BLOCK_BASE          ((u32 *) 0x040000)
#define FLASH_BLOCK_SIZE          ((u32) 0x040000)

#define MAIN_BLOCK_SIZE           0x20000
#define MAIN_BLOCK_WORDS          (MAIN_BLOCK_SIZE / 2)
#define MAIN_BLOCK_DWORDS         (MAIN_BLOCK_WORDS / 2)
#define FLASH_MAX                 (MAIN_BLOCK_SIZE * NUM_FLASH_BLOCKS)

#define KERNEL_START              (MAIN_BLOCK_SIZE * 3)       //first block
#define CE_BIN_START              KERNEL_START

#endif //INTERLEAVED

#define DEREF(x) ((status.flash_chip == i128) ? (*(volatile u16 *)x) : (*(volatile u32 *)x))
#define FLASH_BUS_WIDTH ((status.flash_chip == i128) ? (sizeof(u16)) : \
		                                       (sizeof(u32)))

#define CAST(x) ((status.flash_chip == i128) ? ((u16)x) : ((u32)x))

typedef enum {
        blRamdisk, blKernel, blBloader
} tBlockType;

void init_flash(void);
int block_erase_flash(u32 *addr);
int block_write_flash_sector(u32 *dest, u32 *src);
int block_write_flash(u32 *dest, u32 *source, int size, int perm);
int block_read_flash(u32 uDestAddr, u32 uSrcAddr, u32 uSize);
int get_device_info_flash (u32 uBlockAddr, u32 *pManufacturerID,
    u32 *pDeviceID, u32 *pBlockSize, u32 *pDeviceSize);
int wait_flash(volatile u32 *flash);

#endif //FLASH_I128_H
