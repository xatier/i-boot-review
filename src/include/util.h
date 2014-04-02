//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      util.h
//
// Description:
//
//      Various stdlib like utility functions.
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

#ifndef UTIL_H
#define UTIL_H

// Number of elements in an array
#define countof(a) (sizeof(a) / sizeof((a)[0]))

#define RAND_MAX 2147483647

#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

void itc_memcpy(u8 *dest, u8 const *src, int size);
void itc_memmove(u8 *dest, u8 const *src, int size);
void memset8(u8 *address, u8 value, int length);
void memset16(u16 *address, u16 value, int length);
void memset32(u32 *address, u32 value, int length);
int itc_memcmp(u8 const *src1, u8 const *src2, int length);
int cmp32(u32 const *src1, u32 const *src2, int length);
void print_bytes(u8 const *start, int size);
void print_words(u16 const *start, int size);
void print_dwords(u32 const *start, int size);
void srand(unsigned int seed);
int rand(void);

#endif
