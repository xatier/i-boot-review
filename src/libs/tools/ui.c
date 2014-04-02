//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ui.c
//
// Description:
//
//      Provides a user interface over the serial port.
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

#include <serial.h>
#include <ui.h>
#include <parser.h>
#include <string.h>
#include <util.h>
#include <timer.h>
#include <nvconfig.h>

// Special key to hit in order to skip EEPROM configuration processing
#define SKIP_EEPROM_KEY ('C' - 0x40)    // Ctrl-C

////////////////////////////////////////////////////////////////////////////////
// prompt_ui
// PURPOSE: Prints a prompt based on the current mode (partition or default).
// PARAMS:  (IN) mode cur_mode - the current mode.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
prompt_ui(mode cur_mode)
{
   switch(cur_mode)
   {
      case mode_default:
      {
         itc_printf("IBoot> ");
         break;
      }
      case mode_partition:
      {
         itc_printf("IBoot/Part> ");
         break;
      }
      default:
      {
         itc_printf("IBoot> ");
         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// init_ui
// PURPOSE: Brings up the serial interface with a timeout. If the timeout is
//          exceeded without any input, init_ui returns. If timeout is 0, the
//          interface is brought up immediately without a timer.
// PARAMS:  (IN) int timeout - time in seconds to wait for input before
//                             returning
//          (IN) mode cur_mode - mode to start in.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
init_ui(int timeout,
        mode cur_mode)
{
   char commandline[MAX_COMMAND_SIZE];
   char temp = 0;

   commandline[0] = '\x0';
   if(timeout != 0)
   {
      for ( ; timeout; --timeout)
      {
         itc_printf("\b%i", timeout);
         delay(1);
         if(input_byte_serial(&temp))
	 {
            break;
	 }
      }
      itc_printf("\b%i\r\n", timeout);

      if(timeout == 0)
      {
         // Read the EEPROM and initialize variables (especially CPU speed)
#ifdef TAGGED_EEPROM
         nv_setup();
#endif
         return;
      }
   }

#ifdef TAGGED_EEPROM
   if (temp != SKIP_EEPROM_KEY)
   {
       // If we were called with timeout == 0 it means we just failed to boot
       // an OS, so skip reading the configuration because we did already it.
       if (timeout > 0)
       {
           // Read the EEPROM and initialize variables
           nv_setup();
       }
   } else
   {
       itc_printf("Bypassing reading EEPROM configuration\r\n");
   }
#endif

   while(cur_mode != mode_quit)
   {
      prompt_ui(cur_mode);
      input_line_serial(commandline, MAX_COMMAND_SIZE);
      cur_mode = parse_command(commandline, cur_mode);
   }
}
