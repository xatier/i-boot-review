//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      string.c
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

#include <stdarg.h>
#include <types.h>
#include <serial.h>
#include <util.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// cmpstr
// PURPOSE: Checks for a token at the start of a string, for the purpose of
//          extracting a command from a commandline.
// PARAMS:  (IN) char *str   - string to search in.
//          (IN) char *token - token to search for.
// RETURNS: int - 1 for match, 0 for non match
////////////////////////////////////////////////////////////////////////////////
int
cmpstr(char const *str,
       char const *token)
{
   if(*str == 0 && *token == 0)
   {
      return 1;
   }
   do
   {
      if(*str++ != *token++)
      {
         return 0;
      }
   } while(*token != 0);

   if(*str == ' ' || *str == 0 || *str == ':')
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

////////////////////////////////////////////////////////////////////////////////
// itc_strcmp
// PURPOSE: Checks for a string at the beginning of another string.
// PARAMS:  (IN) char *str   - string to search in.
//          (IN) char *token - token to search for.
// RETURNS: int - 1 for match, 0 for non match
////////////////////////////////////////////////////////////////////////////////
int
itc_strcmp(char const *str1,
	   char const *str2)
{
   while(*str1++ == *str2++ && *str1 != 0){;}
   if(*str1 == 0)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

////////////////////////////////////////////////////////////////////////////////
// itc_strlen
// PURPOSE: Returns the length, in characters, of a given string.
// PARAMS:  (IN) char *str - string to count.
// RETURNS: int - number of characters in string.
////////////////////////////////////////////////////////////////////////////////
int
itc_strlen(char const *str)
{
   int i = 0;

   while(*str++ != '\0')
   {
      ++i;
   }

   return i;
}

////////////////////////////////////////////////////////////////////////////////
// itc_strlcpy
// PURPOSE: Copies a string up to the maximum length given and nul terminates
// PARAMS:  (OUT) char *dest - buffer to copy into.
//          (IN)  char *src  - string to copy.
//          (IN)  size_t size- size of dest buffer
// RETURNS: Length of the src string.
// NOTES:   Unlike OpenBSD's strlcpy, this one requires size > 0
////////////////////////////////////////////////////////////////////////////////
size_t
itc_strlcpy(char *dest,
            char const *src,
            size_t size)
{
   size_t count = 0;
   // Leave space for nul termination
   --size;
   while(*src != 0 && count < size)
   {
      *dest++ = *src++;
      ++count;
   }

   // Nul terminate the destination
   *dest = 0;

   // Finish counting the length of the source
   while(*src++ != 0)
   {
        ++count;
   }

   return count;
}

////////////////////////////////////////////////////////////////////////////////
// itc_strcpy
// PURPOSE: Copies a string
// PARAMS:  (OUT) char *dest - buffer to copy into.
//          (IN)  char *src  - string to copy.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
itc_strcpy(char *dest,
           char const *src)
{
   while(*src != 0)
   {
      *dest++ = *src++;
   }
   *dest = 0;
}

////////////////////////////////////////////////////////////////////////////////
// itc_printf
// PURPOSE: Prints data based on a formatted string.
// PARAMS: (IN) char *format - String with codes in it.
//         (IN) ...          - variable list, based on the codes in format.
// NOTES:  Supports:
//         %i,%d - integer in decimal format.
//         %s    - string
//         %x    - u32 in hexadecimal format.
//         %p    - void * in hexadecimal format.
//         %c    - ASCII character
//         %I    - same as %i, except that it splices into the string,
//                 overwriting to the right of it.
////////////////////////////////////////////////////////////////////////////////
void
itc_printf(char const *format,
           ...)
{
   va_list vl;
   char buf[MAX_STRING_SIZE];
   char *formatc = buf;

   itc_strlcpy(formatc, format, MAX_STRING_SIZE);

   va_start(vl, format);
   for(;*formatc != 0;formatc++)
   {
      if(*formatc != '%')
      {
         output_byte_serial(*formatc);
      }
      else
      {
         switch(*++formatc)
         {
            case 'd':
            case 'i':
            {
               char temp[12];

               temp[itoa(va_arg(vl, int), (char *)temp)] = 0;
               output_string_serial((char *)temp);
               break;
            }
            case 'I':
            {
               itoa(va_arg(vl, int), formatc + 1);
               break;
            }
            case 'p':
            case 'x':
            {
               char temp[9];

               u32toa(va_arg(vl, u32), (char *)temp);
               temp[8] = 0;
               output_string_serial((char *)temp);
               break;
            }
            case 'c':
            {
               output_byte_serial(va_arg(vl, u8));
               break;
            }
            case 's':
            {
               output_string_serial(va_arg(vl, char *));
               break;
            }
            case 'b':
            {
               char temp[3];

               u8toa(va_arg(vl, u8), temp);
               output_string_serial(temp);
               break;
            }
            default:
            {
               // Unsupported format character, or %%
               output_byte_serial(*formatc);
               break;
            }
         }
      }
   }
   va_end(vl);
}

////////////////////////////////////////////////////////////////////////////////
// itoa
// PURPOSE: Converts an int into its ASCII decimal representation.
// PARAMS:  (IN)  int number - number to convert.
//          (OUT) char *buf  - buffer to write string into.
// RETURNS: int - number of characters written into buf.
// NOTES:   Does NOT null terminate the string, to allow for replacing %I and
//          trailing spaces with the string in itc_printf.
//          Does NOT work when number == INT_MIN
////////////////////////////////////////////////////////////////////////////////
int
itoa(int number,
     char *buf)
{
   int divisor;
   int digit = 0;
   int started = 0;
   int i = 0;

   if (number < 0)
   {
      // Convert negative number to positive to display
      // Note that this will fail for the most negative possible integer
      // because that one number doesn't have a positive equivalent.
      *buf++ = '-';
      ++i;
      number = -number;
   }

   for(divisor = 1000000000; divisor > 0; divisor /= 10)
   {
      digit = number / divisor;
      number %= divisor;
 
      if(digit >= 0 && digit < 10)
      {
         if(started || digit > 0)
         {
            *buf++ = (char)('0' + digit);
            started = 1;
            i++;
         }
      }
   }

   if(!started)
   {
      *buf = '0';
      return 1;
   }

   return i;
}

////////////////////////////////////////////////////////////////////////////////
// u32toa
// PURPOSE: Converts a u32 into its ASCII hexadecimal representation.
// PARAMS:  (IN)  u32 number - number to convert.
//          (OUT) char *buf  - buffer to write string into.
// RETURNS: Nothing. (Number of characters is always 8.)
// NOTES:   Does NOT null terminate the string, to allow for replacing %X and
//          trailing spaces with the string in itc_printf.
////////////////////////////////////////////////////////////////////////////////
void
u32toa(u32 number,
       char *buf)
{
   int i;

   for(i = (sizeof(u32) * 2) - 1;i >= 0;i--)
   {
      u32 temp = (number >> (i * 4)) & 0x0F;
      *buf++ = temp + ((temp <= 9) ? '0' : 'A' - 10);
   }
}

#if 0
////////////////////////////////////////////////////////////////////////////////
// atou8
// PURPOSE: Converts an ASCII hexadecimal string into a u8.
// PARAMS:  (IN)  char *number - number to convert.
//          (OUT) u8 *value    - Number as a u8.
// RETURNS: Number of characters converted.
////////////////////////////////////////////////////////////////////////////////
int
atou8(char const *number,
      u8 *value)
{
   int count = sizeof(u8) * 2;
 
   *value = 0;
 
   while(*number != 0 && count)
   {
      if(*number >= '0' && *number <= '9')
      {
         *value <<= 4;
         *value += *number - '0';
      }
      else if(*number >= 'A' && *number <= 'F')
      {
         *value <<= 4;
         *value += *number - 'A' + 10;
      }
      else if(*number >= 'a' && *number <= 'f')
      {
         *value <<= 4;
         *value += *number - 'a' + 10;
      }
      else
      {
         break;
      }

      number++;
      count--;
   }
   return (sizeof(u8) * 2) - count;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// atou16
// PURPOSE: Converts an ASCII hexadecimal string into a u16.
// PARAMS:  (IN)  char *number - number to convert.
//          (OUT) u16 *value    - Number as a u16.
// RETURNS: Number of characters converted.
////////////////////////////////////////////////////////////////////////////////
int
atou16(char const *number,
       u16 *value)
{
   int count = sizeof(u16) * 2;

   *value = 0;

   while(*number != 0 && count)
   {
      if(*number >= '0' && *number <= '9')
      {
         *value <<= 4;
         *value += *number - '0';
      }
      else if(*number >= 'A' && *number <= 'F')
      {
         *value <<= 4;
         *value += *number - 'A' + 10;
      }
      else if(*number >= 'a' && *number <= 'f')
      {
         *value <<= 4;
         *value += *number - 'a' + 10;
      }
      else
      {
         break;
      }

      number++;
      count--;
   }
   return (sizeof(u16) * 2) - count;
}

////////////////////////////////////////////////////////////////////////////////
// atou32
// PURPOSE: Converts an ASCII hexadecimal string into a u32.
// PARAMS:  (IN)  char *number - number to convert.
//          (OUT) u32 *value    - Number as a u32.
// RETURNS: Number of characters converted.
////////////////////////////////////////////////////////////////////////////////
int
atou32(char const *number,
       u32 *value)
{
   int count = sizeof(u32) * 2;
 
   *value = 0;

   while(*number != 0 && count)
   {
      if(*number >= '0' && *number <= '9')
      {
         *value <<= 4;
         *value += *number - '0';
      }
      else if(*number >= 'A' && *number <= 'F')
      {
         *value <<= 4;
         *value += *number - 'A' + 10;
      }
      else if(*number >= 'a' && *number <= 'f')
      {
         *value <<= 4;
         *value += *number - 'a' + 10;
      }
      else
      {
         break;
      }

      number++;
      count--;
   }
   return (sizeof(u32) * 2) - count;
}

////////////////////////////////////////////////////////////////////////////////
// atoip
// PURPOSE: Converts an ASCII IP address (e.g. 192.168.1.1) to a u32.
// PARAMS:  (IN)  char *ip - IP address to convert.
// RETURNS: IP address as a u32.
////////////////////////////////////////////////////////////////////////////////
u32
atoip(char const *ip)
{
   u32 result = 0;
   u32 temp = 0;
   int i = 0;

   do
   {
      if(*ip >= '0' && *ip <= '9')
      {
         temp *= 10;
         temp += *ip - '0';
      }
      if(*ip == '.' || *ip == 0 || *ip == ' ')
      {
	 if(temp > 255)
	 {
	    return 0;
	 }
         i++;
         result <<= 8;
         result |= temp;
         temp = 0;
      }
   } while(*ip != 0 && *ip++ != ' ');

   if(i == 4)
   {
      return result;
   }
   else
   {
      return 0;
   }
}

////////////////////////////////////////////////////////////////////////////////
// iptoa
// PURPOSE: Returns a static string with an IP address in dotted decimal
// PARAMS:  (IN) u32 addr - IP address
// RETURNS: pointer to static string
////////////////////////////////////////////////////////////////////////////////
char const *
iptoa(u32 addr)
{
   static char ipstring[17];

   // itoa doesn't null terminate the strings so do it now
   memset8(ipstring, 0, sizeof(ipstring));
   itoa((u8)((addr & 0xFF000000) >> 24), ipstring);
   ipstring[itc_strlen(ipstring)] = '.';
   itoa((u8)((addr & 0x00FF0000) >> 16), ipstring+itc_strlen(ipstring));
   ipstring[itc_strlen(ipstring)] = '.';
   itoa((u8)((addr & 0x0000FF00) >> 8), ipstring+itc_strlen(ipstring));
   ipstring[itc_strlen(ipstring)] = '.';
   itoa((u8)(addr), ipstring+itc_strlen(ipstring));
   return ipstring;
}

////////////////////////////////////////////////////////////////////////////////
// atoversion
// PURPOSE: Converts an ASCII dotted version quartet into 4 u8s.
// PARAMS:  (IN)  char *version - ASCII dotted quartet.
//          (OUT) u8 *major     - First number.
//          (OUT) u8 *minor     - Second number.
//          (OUT) u8 *build     - Third number.
//          (OUT) u8 *relinfo   - Fourth number.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
atoversion(char const *version,
           u8 *major,
           u8 *minor,
           u8 *build,
           u8 *relinfo)
{
   u8 *current;
   int i = 0;
 
   current = major;
   *current = 0;

   do
   {
      if(*version >= '0' && *version <= '9')
      {
         *current *= 10;
         *current += *version - '0';
      }
      if(*version == '.')
      {
         if(current == major)
	 {
            current = minor;
	 }
         else if(current == minor)
	 {
            current = build;
	 }
         else if(current == build)
	 {
            current = relinfo;
	 }
         *current = 0;
         i++;
      }
   } while(*version++ != 0);

   return (i < 3 ? 0 : 1);
}

////////////////////////////////////////////////////////////////////////////////
// u8toa
// PURPOSE: Converts a u8 into its ASCII hexadecimal .
// PARAMS:  (IN)  u8 number    - number to convert.
//          (OUT) char *result - buffer to write ASCII to.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
u8toa(u8 number,
      char *result)
{
   int digit, i;
 
   for(i = 4; i >= 0; i -= 4)
   {
      digit = (((0x0F << i) & number) >> i);
      if(digit < 10)
      {
         *result++ = '0' + digit;
      }
      else
      {
         *result++ = 'A' + digit - 10;
      }
   }
   *result = 0;
}

////////////////////////////////////////////////////////////////////////////////
// u16toa
// PURPOSE: Converts a u16 into its ASCII hexadecimal .
// PARAMS:  (IN)  u16 number   - number to convert.
//          (OUT) char *result - buffer to write ASCII to.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
u16toa(u16 number,
       char *result)
{
   int digit, i;

   for(i = 12; i >= 0; i -= 4)
   {
      digit = (((0x000F << i) & number) >> i);
      if(digit < 10)
      {
         *result++ = '0' + digit;
      }
      else
      {
         *result++ = 'A' + digit - 10;
      }
   }
   *result = 0;
}

////////////////////////////////////////////////////////////////////////////////
// chopstr
// PURPOSE: Converts a string ending in a '\r' or '\n' into a null terminated
//          string.
// PARAMS:  (IN/OUT) char *string - string to chop.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
chopstr(char *string)
{
   while(*string != 0)
   {
      if(*string == '\r' || *string == '\n')
      {
         *string = 0;
         break;
      }
      string++;
   }
}

////////////////////////////////////////////////////////////////////////////////
// atoi
// PURPOSE: Converts a decimal string into an int.
// PARAMS:  (IN)  char *number - decimal string to convert.
//          (OUT) int value    - resulting int.
// RETURNS: Number of characters converted.
////////////////////////////////////////////////////////////////////////////////
int
atoi(char const *number,
     int *value)
{
   int i = 0;

   *value = 0;

   while(*number >= '0' && *number <= '9')
   {
      *value *= 10;
      *value += *number - '0';
      number++;
      i++;
   }
   return i;
}

////////////////////////////////////////////////////////////////////////////////
// next_token
// PURPOSE: Skips to the first character of the next token.
// PARAMS:  char *string - string to parse.
// RETURNS: char * - pointer to offset in string.
////////////////////////////////////////////////////////////////////////////////
char const
*next_token(char const *string)
{
   if(*string == ':')
   {
      string++;
   }

   //skip to the next whitespace, skipping over backslash escaped characters
   while(*string != 0 && !(*string == ' ' || *string == ':'))
   {
      if (*string == '\\' && *(string+1))
      {
         string++;
      }
      string++;
   }

   //and pass it over
   while(*string != 0 && *string == ' ')
   {
      string++;
   }

   return string;
}
