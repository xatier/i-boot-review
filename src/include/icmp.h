//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      icmp.h
//
// Description:
//
//      Provides Internet Control Message Protocol interface functions.
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

#ifndef ICMP_H
#define ICMP_H

#include "types.h"

// ICMP protocol message types
#define ICMP_ECHOREPLY          0       // Echo Reply
#define ICMP_ECHO               8       // Echo Request
#define ICMP_DEST_UNREACH       3       // Destination Unreachable

// Codes for ICMP_DEST_UNREACH
#define ICMP_PROT_UNREACH       2       // Protocol Unreachable
#define ICMP_PORT_UNREACH       3       // Port Unreachable

// Byte offsets into ICMP packet
#define ICMP_TYPE_OFFSET		(0)
#define ICMP_CODE_OFFSET		(1)
#define ICMP_CHECKSUM_OFFSET		(2)
#define ICMP_DATA_OFFSET		(4)

#define ICMP_HEADER_SIZE		(4)

// Number of bytes of the datagram to copy for ICMP_PROT_UNREACH et. al.
#define ICMP_DATAGRAM_COPY      8

// Largest ICMP packet we will send
#define ICMP_SIZE       (ETHER_HEADER_SIZE+IP_HEADER_SIZE+ICMP_HEADER_SIZE+\
                         IP_HEADER_SIZE+32)

void icmppacket(u8 *packet, u16 length, u32 daddr, u32 saddr,
          u16 type, u16 code, u16 *mymac);
void icmpreply(u8 *dest, u16 size);
void icmpunreachable(u8 *dest, u16 size, u8 code);
void icmpping(u32 iaddr);

#endif //ICMP_H
