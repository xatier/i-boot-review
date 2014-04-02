//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      timer.h
//
// Description:
//
//      Function definitions for SA1110, or Xscale timers.
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

#ifndef TIMER_H
#define TIMER_H

#include <types.h>

#ifdef XSCALE
#define RCNR         (*(volatile u32 *)(0x40900000))

#else
#define RTSR         (*(volatile u32 *)(0x90010010))
#define RCNR         (*(volatile u32 *)(0x90010004))
#define POSR         (*(volatile u32 *)(0x9002001C))
#endif

void init_timer(void);
u32 get_time_timer(void);
void delay(int time);
void udelay (unsigned int microseconds);

#endif //TIMER_H
