//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      net.h
//
// Description:
//
//      Various network related constants and macros.
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

#ifndef NET_H
#define NET_H

// Network byte order conversion macros for little-endian machines
#define htons(x)                        (u16)(((u16)(x) << 8) | ((u16)(x) >> 8))
#define ntohs(x)                        htons(x)
#define htonl(x)                        (u32)((((x) & 0x000000FF) << 24) | \
                                              (((x) & 0x0000FF00) << 8)  | \
                                              (((x) & 0x00FF0000) >> 8)  | \
                                              (((x) & 0xFF000000) >> 24))
#define ntohl(x)                        htonl(x)

#define ETHER_HEADER_SIZE               (14)
#define IP_HEADER_SIZE                  (20)

#define MIN_PACKET_SIZE			(64)
#define MAX_PACKET_SIZE			(1518)

#define IP_BROADCAST                    (0xFFFFFFFF)

#define NET_TIMEOUT                     (5)
#define ARP_TIMEOUT                     (1)     // ARP should reply quickly

// Ports less than this value are reserved for privileged processes.
#define IPPORT_RESERVED                 (1024)

#endif //NET_H
