//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      os.h
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

#ifndef OS_H
#define OS_H

#include <types.h>

#define LINUX_SIG	0x016F2818
#define CE_CLEAR_SIZE	0x200000

void init_os(void);
void run_os(char const * arg);
void boot_linux_kernel( char const *arg, u8 *addr);

#endif //OS_H
