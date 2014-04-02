//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      serial_sa.c
//
// Description:
//
//      Interfaces with the StrongArm SA1110 serial hardware.
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
#include <timer.h>

////////////////////////////////////////////////////////////////////////////////
// init_serial
// PURPOSE: Initializes the debug serial port.
// PARAMS:  None.
// RETURNS: Nothing
////////////////////////////////////////////////////////////////////////////////
void
init_serial(u32 baud)
{
   //wait for the fifo to clear
   while(SERIAL_UTSR1 & UTSR1_TX_BSY);

   //turn everything off
   SERIAL_UTCR3 = 0;
   SERIAL_UTCR0 = 0xFF;

   SERIAL_UTCR0 = (UTCR0_1_SBIT | UTCR0_8_DBIT);

   //set the speed we want
   SERIAL_UTCR1 = 0;
   SERIAL_UTCR2 = baud; //SERIAL_BAUD_38400;

   //Turn everything back on
   SERIAL_UTCR3 = (UTCR3_RX_ON | UTCR3_TX_ON);
}

////////////////////////////////////////////////////////////////////////////////
// output_byte_serial
// PURPOSE: Prints a byte to the serial port.
// PARAMS:  (IN) char byte - byte to print.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
output_byte_serial(char byte)
{
   //wait for room in the fifo.
   while((SERIAL_UTSR0 & UTSR0_TX_HALF_EMPTY) == 0);

   SERIAL_UTDR = byte;
}

////////////////////////////////////////////////////////////////////////////////
// input_byte_serial
// PURPOSE: Reads a byte from the serial port.
// PARAMS:  (OUT) char *byte - byte read.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
input_byte_serial(char *byte)
{
   int error = 0;

   if(SERIAL_UTSR1 & UTSR1_RX_NOT_EMPTY)
   {
      error = (SERIAL_UTSR1 &
               (UTSR1_RX_PARITY_ERROR |
                UTSR1_RX_FRAMING_ERROR |
                UTSR1_RX_OVERFLOW));
      if(error)
      {
         SERIAL_UTCR3 = 0;
         SERIAL_UTCR3 = (UTCR3_RX_ON | UTCR3_TX_ON);
         return 0;
      }

      *byte = (char)SERIAL_UTDR;
      return 1;
   }
   else
   {
      return 0;
   }
}
