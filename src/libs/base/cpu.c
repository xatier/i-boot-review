//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      cpu_xscale.c
//
// Description:
//
//      CPU settings.
//
// Author:
//
//      Dan Fandrich
//
// Created:
//
//      September 2002
//
////////////////////////////////////////////////////////////////////////////////

#include <types.h>
#include <timer.h>
#include <cpu.h>

extern void freq_change(u32 cccr);

////////////////////////////////////////////////////////////////////////////////
// get_speed_multiplier
// PURPOSE: Return the multiplier value for the given CPU speed
// PARAMS:  (IN/OUT) u32 *freq - CPU frequency in MHz
//          (OUT) u32 *mul - multiplier
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
static inline int
get_speed_multiplier(u32 *freq, u32 *mult)
{
#ifdef XSCALE
    // Value for CCCR
    static const struct {u32 freq; u32 mult;} speed_table[] = {
        {400, 0x241},
        {300, 0x1C1},
        {200, 0x141},
        //{150, 0x1A1}, // Not supported--see Intel errata #121
        //{100, 0x121}, // Not supported--see Intel errata #121
        {0, 0}  // end of table
    };
#else
    // Value for CCF
    static const struct {u32 freq; u32 mult;} speed_table[] = {
        {221, 0x0B},
        {206, 0x0A},
        {192, 0x09},
        {177, 0x08},
        {162, 0x07},
        {148, 0x06},
        {133, 0x05},
        {118, 0x04},
        {103, 0x03},
        {89, 0x02},
        {74, 0x01},
        {59, 0x00},
        {0, 0}
    };
#endif
    int i = 0;

    for (i=0; speed_table[i].freq > *freq; ++i)
        ; // search table

    *freq = speed_table[i].freq;
    *mult = speed_table[i].mult;

    return (*freq > 0);
}

////////////////////////////////////////////////////////////////////////////////
// set_cpu_speed
// PURPOSE: Sets the CPU speed
// PARAMS:  (IN) int speed - speed in MHz
// RETURNS: CPU speed actually set, or < 0 for invalid speed
////////////////////////////////////////////////////////////////////////////////
int
set_cpu_speed(u32 speed)
{
   u32 multiplier;

   if (!get_speed_multiplier(&speed, &multiplier))
   {
       return -1;
   }
   // Wait for serial port to flush (only needed on SA-1110)
   udelay(10000);

   freq_change(multiplier);

   // Wait for PLL to stabilize before writing to serial port (only
   // needed on SA-1110)
   udelay(500);

   return speed;
}

