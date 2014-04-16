//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      util.c
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

#include <types.h>
#include <checksum.h>
#include <string.h>
#include <util.h>

static int random = 1;

void memcpy(u8 *dest, u8 const *src, int size);

////////////////////////////////////////////////////////////////////////////////
// itc_memcpy
// PURPOSE: Copys memory from one location to another.
// PARAMS:  (IN) u8 *dest - Destination address.
//          (IN) u8 *src  - Source address.
//          (IN) int size - Number of bytes to copy.
////////////////////////////////////////////////////////////////////////////////
void
itc_memcpy(u8 *dest,
           u8 const *src,
           int size)
{
   /* if src and dest are 32-bit aligned, copy 32-bit words instead of bytes */
   if (((u32)dest | (u32)src) % sizeof(u32) == 0)
   {
      while (size >= sizeof(u32))
      {
         *((u32 *)dest)++ = *((u32 *)src)++;
         size -= sizeof(u32);
      }
   }
   /* copy any remaining bytes */
   while(size--)
   {
      *dest++ = *src++;
   }
}

////////////////////////////////////////////////////////////////////////////////
// memcpy
// PURPOSE: GCC insists on having a memcpy for initializing some arrays, and
//          will fail to link without a memcpy symbol.
// PARAMS:  (IN) u8 *dest - Destination address.
//          (IN) u8 *src  - Source address.
//          (IN) int size - Number of bytes to copy.
////////////////////////////////////////////////////////////////////////////////
void
memcpy(u8 *dest,
       u8 const *src,
       int size)
{
   itc_memcpy(dest, src, size);
}

////////////////////////////////////////////////////////////////////////////////
// itc_memmove
// PURPOSE: Moves memory (overlapping ok) from one location to another.
// PARAMS:  (IN) u8 *dest - Destination address.
//          (IN) u8 *src  - Source address.
//          (IN) int size - Number of bytes to copy.
////////////////////////////////////////////////////////////////////////////////
void
itc_memmove(u8 *dest,
       u8 const *src,
       int size)
{
   if( dest < src)
      itc_memcpy(dest, src, size);
   else
   {
      dest += size;
      src  += size;

      while(size--)
      {
	 *--dest = *--src;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// memset8
// PURPOSE: Sets 8 bit aligned memory to an 8 bit pattern.
// PARAMS:  (IN) u8 *address - address to start at
//          (IN) u8 value    - value to set memory to.
//          (IN) int length  - number of bytes to set.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
memset8(u8 *address,
        u8 value,
        int length)
{
   while(length-- > 0)
   {
      *address++ = value;
   }
}
 
////////////////////////////////////////////////////////////////////////////////
// memset16
// PURPOSE: Sets 16 bit aligned memory to a 16 bit pattern.
// PARAMS:  (IN) u16 *address - address to start at
//          (IN) u16 value    - value to set memory to.
//          (IN) int length  - number of words to set.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
memset16(u16 *address,
         u16 value,
         int length)
{
   while(length-- > 0)
   {
      *address++ = value;
   }
}
 
////////////////////////////////////////////////////////////////////////////////
// memset32
// PURPOSE: Sets 32 bit aligned memory to a 32 bit pattern.
// PARAMS:  (IN) u32 *address - address to start at
//          (IN) u32 value    - value to set memory to.
//          (IN) int length   - number of dwords to set.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
memset32(u32 *address,
         u32 value,
         int length)
{
   while(length-- > 0)
   {
      *address++ = value;
   }
}

////////////////////////////////////////////////////////////////////////////////
// itc_memcmp
// PURPOSE: Compares 2 memory regions.
// PARAMS:  (IN) u8 *src1  - First memory region.
//          (IN) u8 *src2  - Second memory region.
//          (IN) int length - length of memory regions.
// RETURNS: -1 if src1 < src2, 0 for match, 1 for src1 > src2
////////////////////////////////////////////////////////////////////////////////
int
itc_memcmp(u8 const *src1,
           u8 const *src2,
           int length)
{
   while((*src1 == *src2) && (length > 0))
   {
      length--;
      src1++;
      src2++;
   }

   return (length > 0) ? ((*src1 > *src2) * 2 - 1) : 0;
}

////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Compares 2 memory regions.
// PARAMS:  (IN) u32 *src1  - First memory region.
//          (IN) u32 *src2  - Second memory region.
//          (IN) int length - length of memory regions.
// RETURNS: 1 for match, 0 for non match.
////////////////////////////////////////////////////////////////////////////////
int
cmp32(u32 const *src1,
      u32 const *src2,
      int length)
{
   while((*src1++ == *src2++) && (length > 0))
   {
      length--;
   }

   return ((length > 0) ? 0 : 1);
}

////////////////////////////////////////////////////////////////////////////////
// print_bytes
// PURPOSE: Prints the values of size bytes.
// PARAMS:  (IN) u8 *start - starting address.
//          (IN) int size  - number of bytes to print.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
print_bytes(u8 const *start,
            int size)
{
   int x = 1;
 
   while(size > 0)
   {
      itc_printf("%b", *start++);
      size--;
      if(!(x++ % 27))
      {
         if(size > 0)
         {
            itc_printf("\r\n");
            x = 1;
         }
      }
      else
      {
         itc_printf(" ");
      }
   }
   itc_printf("\r\n");
}

////////////////////////////////////////////////////////////////////////////////
// print_words
// PURPOSE: Prints the values of size words.
// PARAMS:  (IN) u8 *start - starting address.
//          (IN) int size  - number of words to print.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
print_words(u16 const *start,
            int size)
{
   int x = 1;
 
   while(size > 0)
   {
      char buf[5];

      u16toa(*start++, buf);
      itc_printf(buf);
      size--;
      if(!(x++ % 16))
      {
         if(size > 0)
         {
            itc_printf("\r\n");
            x = 1;
         }
      }
      else
      {
         itc_printf(" ");
      }
   }
   itc_printf("\r\n");
}

////////////////////////////////////////////////////////////////////////////////
// print_dwords
// PURPOSE: Prints the values of size dwords.
// PARAMS:  (IN) u8 *start - starting address.
//          (IN) int size  - number of dwords to print.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
print_dwords(u32 const *start,
            int size)
{
   int x = 1;
 
   while(size > 0)
   {
      itc_printf("%x", *start++);
      size--;
      if(!(x++ % 8))
      {
         if(size > 0)
         {
            itc_printf("\r\n");
            x = 1;
         }
      }
      else
      {
         itc_printf(" ");
      }
   }
   itc_printf("\r\n");
}

////////////////////////////////////////////////////////////////////////////////
// srand
// PURPOSE: Seeds the random number generator
// PARAMS:  (IN) unsigned int seed - seed number
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void srand(unsigned int seed)
{
    random = seed;
}

////////////////////////////////////////////////////////////////////////////////
// rand
// PURPOSE: Returns a pseudo-random number between 0 and RAND_MAX
// PARAMS:  None.
// RETURNS: Pseudo-random number
// NOTES:   Overload the CRC-32 functions to generate these numbers.
////////////////////////////////////////////////////////////////////////////////
int rand(void)
{
    static const char static_rand = 0x55;
    return random & RAND_MAX;
}
