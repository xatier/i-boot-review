//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      cebootme.c
//
// Description:
//
//      Broadcasts a "bootme" packet to notify instances of MS Platform Builder
//      running on this subnet of our presence.
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

#include <cebootme.h>
#include <c_main.h>
#include <types.h>
#include <net.h>
#include <udp.h>
#include <arp.h>
#include <util.h>
#include <string.h>
#include <util.h>
#include <string.h>

#define EDBG_CPU_SA1100 0x40

////////////////////////////////////////////////////////////////////////////////
// cebootme
// PURPOSE: Sends a "Bootme" packet to notify instances of MS Platform Builder
//          running on this subnet of our presence.
// PARAMS:  None.
////////////////////////////////////////////////////////////////////////////////
int
cebootme(void)
{
   u8 buf[MAX_PACKET_SIZE];
   u8 *packet = buf;
   bootme *data = (bootme *)(packet + UDPIP_HEADER_SIZE);
   int i = 0, ver;

   memset8(packet, 0, sizeof(bootme) + UDPIP_HEADER_SIZE);

   i = (u32)data % sizeof(u32);
   packet += i;
   (u32)data += i;

   data->protoid = htonl(CE_PROTOID);
   data->service = CE_SERVICE;
   data->flags = CE_FLAGS;
   data->count = 1;
   data->cmd = 0;
   // This is a horrible hack to split the version number into major,minor
   // but should work until version 10.0
   atoi(VERSION, &ver);
   data->majorversion = ver;
   atoi(VERSION + 2, &ver);     // skip over major version and decimal point
   data->minorversion = ver; 
   data->iaddr = htonl(status.ciaddr);
   data->macaddr[0] = status.macaddr[0];
   data->macaddr[1] = status.macaddr[1];
   data->macaddr[2] = status.macaddr[2];
   itc_strcpy(data->id, "uCERF");
   itc_strcpy(data->name, "uCERF_");
   u16toa(status.macaddr[2], (char *)(data->name + 6));
   data->name[11] = '\0';
#ifdef XSCALE
   data->cpuid = EDBG_CPU_SA1100;  // There doesn't seem to be a PXA250 value
#else
   data->cpuid = EDBG_CPU_SA1100;  // SA-1110
#endif

   // The following aren't used if bootmeversion is < 2
   data->bootmeversion = 0;
   data->bootflags = 0;
   data->tftpport = htons(CE_PB_PORT);
   data->srvport = 0;

   return 1;
}
