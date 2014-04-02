//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      uudecode.c
//
// Description:
//
//      Decodes an incoming uuencoded file from the serial port. Adapted from
//      BSD uudecode source.
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

 /*
 ** Copyright (c) 1983 Regents of the University of California.
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 ** 3. Neither the name of the University nor the names of its contributors
 **    may be used to endorse or promote products derived from this software
 **    without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 ** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ** ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 ** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 ** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 ** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 ** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 ** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 ** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 ** SUCH DAMAGE.
 */

#include <types.h>
#include <serial.h>
#include <uudecode.h>

#define DEC(c)	(((c) - ' ') & 077)
#define BUFSIZE 80

////////////////////////////////////////////////////////////////////////////////
// uudecode
// PURPOSE: Decodes an incoming uuencoded file from the serial port.
// PARAMS:  (IN) char *dest - buffer to write file to.
// RETURNS: Number of bytes rxed.
////////////////////////////////////////////////////////////////////////////////
int
uudecode(char *dest)
{
   char buf[BUFSIZE];
   int numbytes = 0;

   //skip the header line - we don't care.
   get_line_serial((char *)buf, BUFSIZE);

   for(;;)
   {
      int n, i, expected, size;
      char *p;

      size = get_line_serial((char *)buf, BUFSIZE);
      n = DEC(buf[0]);
      numbytes += n;

      if(n <= 0 || buf[0] == '\n')
         break;

      expected = ((n+2)/3)<<2;
      for(i = size; i <= expected; i++)
         buf[i] = ' ';

      p = &buf[1];
      while(n > 0)
      {
         char c1 = 0, c2 = 0, c3 = 0;

         c1 = DEC(*p) << 2 | DEC(p[1]) >> 4;
         c2 = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
         c3 = DEC(p[2]) << 6 | DEC(p[3]);

         if(n >= 1)
            *dest++ = c1;
         if(n >= 2)
            *dest++ = c2;
         if(n >= 3)
            *dest++ = c3;
         n -= 3;
         p += 4;
      }
   }

   //skip past the "end" line;
   get_line_serial((char *)buf, BUFSIZE);

   return numbytes;
}
