//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001-2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      serial.c
//
// Description:
//
//      Serial input utility functions.
//
// Author:
//
//      Dan Fandrich
//
// Created:
//
//      June 2002
//
////////////////////////////////////////////////////////////////////////////////

#include <serial.h>
#include <string.h>
#include <timer.h>
#include <idle.h>

////////////////////////////////////////////////////////////////////////////////
// output_string_serial
// PURPOSE: Prints a null terminated string to the serial port.
// PARAMS:  (IN) char *string - string to print.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
output_string_serial(char const *string)
{
   while(*string != 0)
   {
      output_byte_serial(*string++);
      // Call idle occasionally to handle unsolicited packets
      if (*string == '\n')
         idle();
   }
}

////////////////////////////////////////////////////////////////////////////////
// get_line_serial
// PURPOSE: Reads a line (terminated by '\n') from the serial port.
// PARAMS:  (OUT) char *buf  - buffer to write into.
//          (IN)  int buflen - length of buffer in bytes.
// RETURNS: Number of characters read.
////////////////////////////////////////////////////////////////////////////////
int
get_line_serial(char *buf,
                int buflen)
{
   int i = 0;

   do
   {
      while(!input_byte_serial(buf))
         idle();
      i++;
   }
   while(*buf++ != '\r' && *(buf - 1) != '\n' && i < buflen);

   return i;
}

////////////////////////////////////////////////////////////////////////////////
// input_line_serial
// PURPOSE: Reads a line (terminated by '\r') from the serial port.
// PARAMS:  (OUT) char *buf  - buffer to write into (must be NUL terminated).
//          (IN)  int buflen - length of buffer in bytes.
// RETURNS: Number of characters read.
// NOTES:   Unlike get_line_serial, this function echos input back and handles
//          backspace characters sensibly.
////////////////////////////////////////////////////////////////////////////////
int
input_line_serial(char *buf, int buflen)
{
    char *curpos = buf, temp;
    int i = 0, erase = 0;

    do
    {
        while (!input_byte_serial(&temp))
            idle();

        if (temp == '\b' || temp == '\x7f')
        {
            // Erase one character if there is one
            erase = (i > 0);
        }
        else if (temp == 'U' - 0x40)
        {
            // Erase entire line
            erase = i;
        }
        else if (temp == 'P' - 0x40) // Ctrl-P
        {
            // Restore previous line in buffer
            int lastlen = itc_strlen(curpos);
            output_string_serial(curpos);
            i += lastlen;
            curpos += lastlen;
        }
        else if (temp == '\r')  // CR
        {
            *curpos = 0;
            break;              // finished editing
        }
        else if (temp < 0x20)   // any other control character
        {
            // Just beep and ignore it
            output_byte_serial('\007');
        }
        else
        {
            *curpos++ = temp;
            i++;
            output_byte_serial(temp);
            *(curpos+1) = 0;
        }

        // Erase character(s) if requested
        for (; erase; --erase)
        {
            output_string_serial("\b \b");
            *--curpos = 0;
            i--;
        }
    } while ((buflen - i) > 0);

    if ((buflen - i) <= 0)
    {
        output_byte_serial('\r');
        output_byte_serial('\n');
    }
    input_byte_serial(&temp);   // read LF after CR

    output_byte_serial('\r');
    output_byte_serial('\n');

    return i;
}

////////////////////////////////////////////////////////////////////////////////
// raw_input_serial
// PURPOSE: Reads a block from the serial port.
// PARAMS:  (OUT) char *byte - buffer
//          (IN) u32 uCount - size of buffer
//          (IN) int timeout - number of 100ths of seconds to wait for block
// RETURNS: 1 for successful read of all bytes, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
raw_input_serial(char *byte, u32 uCount, int timeout)
{
    int shortdelay = 0;

    while (timeout > 0)
    {
      if (input_byte_serial(byte))
      {
         ++byte;
         if (--uCount == 0)
             return 1;
      } else
      {
          idle();
          udelay(100);
          if (++shortdelay == 100)
          {
              --timeout;
              shortdelay = 0;
          }
      }
    }
    return 0;
}
