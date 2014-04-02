//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      string.h
//
// Description:
//
//      Various string manipulation functions.
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

#ifndef STRING_H
#define STRING_H

#include <types.h>

#define MAX_STRING_SIZE 2048

int cmpstr(char const *str1, char const *str2);
int itc_strcmp(char const *str1, char const *str2);
int itc_strlen(char const *str);
void itc_printf(char const *format, ...);
void itc_strcpy(char *dest, char const *src);
size_t itc_strlcpy(char *dest, char const *src, size_t size);
u32 atoip(char const *ip);
char const * iptoa(u32 addr);
int atou8(char const *number, u8 *value);
int atou16(char const *number, u16 *value);
int atou32(char const *number, u32 *value);
int atoi(char const *number, int *value);
char const *next_token(char const *string);
int itoa(int number, char *buf);
int atoversion(char const *version, u8 *major, u8 *minor, u8 *build, u8 *relinfo);
void u8toa(u8 number, char *result);
void u16toa(u16 number, char *result);
void chopstr(char *string);
void u32toa(u32 number, char *buf);

#endif
