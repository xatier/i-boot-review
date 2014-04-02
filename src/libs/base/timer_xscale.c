//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      timer_xscale.c
//
// Description:
//
//      Interfaces with the Xscale RTC.
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

#include <timer.h>
#include <start_xscale.h>
#include <idle.h>

////////////////////////////////////////////////////////////////////////////////
// init_timer
// PURPOSE: Brings the RTC up to a stable state.
// PARAMS:  None.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
init_timer(void)
{
   //do nothing, because the Xscale RTC is good to go at boot =)
}

////////////////////////////////////////////////////////////////////////////////
// get_time_timer
// PURPOSE: Gets the current time offset, in seconds, since init_timer was run.
// PARAMS:  None.
// RETURNS: u32 - seconds since init_timer.
////////////////////////////////////////////////////////////////////////////////
u32
get_time_timer(void)
{
   return (u32)RCNR;
}

////////////////////////////////////////////////////////////////////////////////
// delay
// PURPOSE: Blocks for a given number of seconds.
// PARAMS:  (IN) int time - number of seconds to delay.
// RETURNS: Nothing.
// NOTES:   Unreliable before init_timer is run.
////////////////////////////////////////////////////////////////////////////////
void
delay(int time){
  u32 now = 0;
 
  now = get_time_timer();
  while(time--)
  {
    while(now == get_time_timer())
    {
       idle();
    }
    now = get_time_timer();
  }
}

////////////////////////////////////////////////////////////////////////////////
// udelay
// PURPOSE: Blocks for a given number of microseconds.
// PARAMS:  (IN) int microseconds - number of microseconds to delay.
// RETURNS: Nothing.
// NOTES:   Max delay is 116000 usec.
////////////////////////////////////////////////////////////////////////////////
void
udelay (unsigned int microseconds)
{
   unsigned int counter = 0;
   /* take OSCR snapshot */
   unsigned int oscr = *(volatile u32 *)OSCR;

   /* 
    * The 32-bit OSCR counter increments at a freq of 3.6863 Mhz
    * and will wrap around every 2^32/3686300 seconds (~19 min)
    */
   unsigned int clk_count = (microseconds * CLK_TO_10MS) / 10000;

   /* check if (oscr+clk_count) wraps around */
   if( ((unsigned int)0xffffffff - clk_count) < oscr)
   {
      /* adjust clk_count */
      clk_count = clk_count - (0xffffffff - oscr);
      while( *(volatile u32 *)OSCR >= oscr)
      {
         ; /* wait until OSCR wraps around */
      }
      oscr = 0;
   }

   for (counter = oscr + clk_count; counter > *(volatile u32 *)OSCR; )
      ;

   return;
}
