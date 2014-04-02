//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      params.h
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

#ifndef PARAMS_H
#define PARAMS_H

#include <platform.h>

//BOOT_PARAMS should match the one in arch/arm/mach*/cerf.c
#define BOOT_PARAMS	(mem_base_platform() + 0x100)
#define MAX_PARAM_SIZE	(1024)
#define NO_BOOT_PARAMS	0

struct tag *kernel_param(char const *commandline);

#endif //PARAMS_H
