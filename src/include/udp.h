//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      udp.h
//
// Description:
//
//      Provides User Datagram Protocol interface functions.
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

#ifndef UDP_H
#define UDP_H

#include "types.h"

#define UDP_PSEUDO_HEADER_OFFSET	(6)

// byte offsets into UDP packet
#define UDP_SRC_PORT_OFFSET		(0)
#define UDP_DEST_PORT_OFFSET		(2)
#define UDP_LENGTH_OFFSET		(4)
#define UDP_CHECKSUM_OFFSET		(6)

#define UDP_HEADER_SIZE			(8)

#define UDPIP_HEADER_SIZE   (ETHER_HEADER_SIZE+IP_HEADER_SIZE+UDP_HEADER_SIZE)

void udppacket(u16 *packet, u16 length, u32 daddr, u32 saddr, u16 dport, u16 sport, u16 *mymac);
int udplisten(u8 *dest, u16 *size, u32 *iaddr, char bcast_enable);

#endif //UDP_H
