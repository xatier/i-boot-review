//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      idle.c
//
// Description:
//
//      Functions to execute when the bootloader is idle.
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

#include <ip.h>
#include <udp.h>
#include <icmp.h>
#include <c_main.h>
#include <idle.h>

////////////////////////////////////////////////////////////////////////////////
// idle
// PURPOSE: Function called when the system is idle
// PARAMS:  Nothing.
// RETURNS: Nothing.
// NOTES:   If the system expects to have a few cycles to spare, this function
//          should be called.  It currently handles uninitiated network
//          packets.
////////////////////////////////////////////////////////////////////////////////
void idle(void)
{
    u8 buf[MAX_PACKET_SIZE];
    //we want the packet to be u16 aligned after the headers (42 bytes)
    u8 * const packet = (u8 *)(buf + sizeof(u16) - 
                              (((u32)buf + UDPIP_HEADER_SIZE) % sizeof(u16)));
    u16 size  = MAX_PACKET_SIZE - sizeof(u16);    // worst case size

    // Only poll for packets if our IP address is configured
    if (status.ciaddr)
    {
    }
}
