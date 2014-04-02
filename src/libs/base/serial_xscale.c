//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      serial_xscale.c
//
// Description:
//
//      Interfaces with the Xscale serial hardware.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      December 2001
//
////////////////////////////////////////////////////////////////////////////////

#include <types.h>
#include <start_xscale.h>
#include <serial.h>

////////////////////////////////////////////////////////////////////////////////
// init_serial
// PURPOSE: Initializes the debug serial port.
// PARAMS:  None.
// RETURNS: Nothing
////////////////////////////////////////////////////////////////////////////////
void
init_serial(u32 baud)
{
   u32 lcr, fcr;

   //wait for the tx fifo to empty
   while(!(*(volatile u32 *)(FFUART_BASE + FFLSR) & 0x00000040))
   {
      ; //do nothing
   }

   //enable access to the latch registers
   lcr = *(volatile u32 *)(FFUART_BASE + FFLCR);
   lcr |= 0x00000080;
   *(volatile u32 *)(FFUART_BASE + FFLCR) = lcr;

   //change the baud rate
   *(volatile u32 *)(FFUART_BASE + FFDLL) = baud;

   //reset fifos
   fcr = *(volatile u32 *)(FFUART_BASE + FFFCR);
   fcr |= 0x00000006;
   *(volatile u32 *)(FFUART_BASE + FFFCR) = fcr;

   //re enable the port
   lcr &= 0xFFFFFF7F;
   *(u32 *)(FFUART_BASE + FFLCR) = lcr;
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
   while((*(volatile u32 *)(FFUART_BASE + FFLSR) & 0x00000020) == 0)
   {
      ; //do nothing
   }

   *(volatile u32 *)(FFUART_BASE + FFTHR) = ((u32)byte & 0xFF);
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
   if((*(volatile u32 *)(FFUART_BASE + FFLSR) & 0x00000001) == 0)
   {
      return 0;
   }
   else
   {
      *byte = *(volatile char *)(FFUART_BASE + FFRBR);
      return 1;
   }
}
