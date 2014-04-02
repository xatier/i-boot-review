//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      platform.h
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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <config.h>

#define PLATFORM_MEMORY_BASE           (mem_base_platform())
#define PLATFORM_MEMORY_MAX            (mem_base_platform() + \
                                        mem_size_platform())
#define PLATFORM_MEMORY_SIZE           (mem_size_platform())
#define PLATFORM_FLASH_BASE            (flash_base_platform())
#define PLATFORM_FLASH_SIZE            (flash_size_platform())

u32 flash_block_size_platform(void);
u32 flash_base_platform(void);
u32 flash_size_platform(void);
u32 mem_size_platform(void);
u32 mem_base_platform(void);

#endif //PLATFORM_H
