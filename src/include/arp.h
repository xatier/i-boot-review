//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      arp.h
//
// Description:
//
//      Handles arp requests and transmissions.
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

#ifndef ARP_H
#define ARP_H

#include <types.h>
#include <c_main.h>

#define ETHER_TYPE_ARP		(0x0608)

#define ARP_HDTYPE		(0x0100)
#define ARP_PROTOCOL		(0x0008)
#define ARP_REQ			(0x0100)
#define ARP_REPLY		(0x0200)

#define ARP_SIZE		300

typedef struct{
 u16 hdtype;
 u16 protocol;
 u8 haddrln;
 u8 protoln;
 u16 opcode;
 u16 macaddr[3];
 u16 iaddr1;
 u16 iaddr2;
 u16 targetmac[3];
 u16 targetiaddr1;
 u16 targetiaddr2;
} __attribute__((packed)) arppacket;

int arp(u32 iaddr, u16 *macaddr);
int arplisten(void);
void arpreply(u8 *packet, u16 size);
void init_arp(void);

#endif //ARP_H
