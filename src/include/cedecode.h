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

#ifndef CEDECODE_H
#define CEDECODE_H

#include "types.h"

#define GETWORD(x) ((*(x + 3) << 24) + (*(x + 2) << 16) + (*(x + 1) << 8) + *x)

#define CE_SIG_SIZE	7

#define CE_SIG_WORD1	0x30303042
#define CE_SIG_WORD2	0x000A4646  //actually only 7 bytes, so mask out the                                        //most significant byte.

int detect_wince_kernel(char const *kernel);
u32 *cedecode(u8 *dest, u8 const *source);

#endif //CEDECODE_H
