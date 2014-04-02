//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ip.h
//
// Description:
//
//      Provides Internet Protocol interface functions.
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

#ifndef IP_H
#define IP_H

#include <types.h>
#include <net.h>

#define IP_VERSION			(4)
#define IP_HEADER_LENGTH		(5)
#define IP_PRECEDENCE			(0x00)
#define IP_ID				(0x0000)
#define IP_CTR_FLAGS			(0x0)
#define IP_OFFSET			(0)
#define IP_TTL				(0x40)
#define IP_TEMP_CHECKSUM		(0x0000)	//temporary checksum
#define IP_ADDRBITLENGTH		(32)

#define IPPROTO_ICMP	        	(0x1)
#define IPPROTO_TCP			(0x6)
#define IPPROTO_UDP			(0x11)

#define IP_HEADER_LENGTH_OFFSET		(0)
#define IP_LENGTH_OFFSET		(2)
#define IP_PROTOCOL_OFFSET		(9)
#define IP_CHECKSUM_OFFSET		(10)
#define IADDR_OFFSET			(12)
#define DADDR_OFFSET			(16)

#define ETHER_TYPE_IP			(0x0008)

void ippacket(u8 *packet, int length, u32 daddr, u32 saddr, u16 *mymac,
              u8 protocol);
u16 ipsum(const char* addr, int count);
void etherpacket(u8 *packet, u16 length, u16 *daddr, u16 *saddr, u16 type);
int iplisten_poll(u8 *data, u16 *size, char bcast_enable);
int iplisten(u8 *data, u16 *size, char bcast_enable);

#endif //IP_H
