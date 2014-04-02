//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      icmp.c
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

#include <icmp.h>
#include <ip.h>
#include <net.h>
#include <util.h>
#include <c_main.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// icmppacket
// PURPOSE: Adds a ICMP header to a packet, and send it over IP.
// IN: u8 *packet - Memory containing packet to send. Data starts 38 bytes in
//                  to leave space for the Ethernet header (14 bytes), IP
//                  header (20 bytes), and ICMP header (4 bytes).
////////////////////////////////////////////////////////////////////////////////
void
icmppacket(u8 *packet,
           u16 length,
           u32 daddr,
           u32 saddr,
           u16 type,
           u16 code,
           u16 *mymac)
{
    u8 * const start = packet + ETHER_HEADER_SIZE + IP_HEADER_SIZE;
    u8 *curpos = start;

    // ICMP header
    *curpos++ = type;
    *curpos++ = code;
    *curpos++ = 0;              // checksum
    *curpos++ = 0;              // checksum

    // Calculate checksum on entire ICMP packet
    *(u16 *) (start + ICMP_CHECKSUM_OFFSET) =
        ipsum(start, length + ICMP_HEADER_SIZE);

    ippacket(packet, length + ICMP_HEADER_SIZE,
             daddr, saddr, mymac,
             IPPROTO_ICMP);
}

////////////////////////////////////////////////////////////////////////////////
// icmpreply
// PURPOSE: Reply to an ICMP packet
// PARAMS:  (IN)  u8 *dest - buffer holding the IP packet.
//          (IN)  u16 size - size of packet.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
icmpreply(u8 *dest, u16 size)
{
    u8 *const packet = dest + IP_HEADER_SIZE;
    u8  buf[MAX_PACKET_SIZE];
    u8 *const start = buf +
                      ETHER_HEADER_SIZE + IP_HEADER_SIZE + ICMP_HEADER_SIZE;
    u32 saddr;

    // Ignore packet if it's too small or has a bad checksum
    if (size < (IP_HEADER_SIZE + ICMP_HEADER_SIZE) ||
        ipsum(packet, size - IP_HEADER_SIZE))
    {
        return;
    }

    saddr = ntohs(*(u16 *) (dest + IADDR_OFFSET + sizeof(u16))) |
        (ntohs(*(u16 *) (dest + IADDR_OFFSET)) << 16);

    switch (*(packet + ICMP_TYPE_OFFSET))
    {
        // Ping echo request packet
        case ICMP_ECHO:
        {
            // Protect against overflowing our buffer on the reply
            size = MIN(size, MAX_PACKET_SIZE - ETHER_HEADER_SIZE - 8);

            // Copy the entire ICMP payload into the reply
            itc_memcpy(start, packet + ICMP_HEADER_SIZE,
                       size - (IP_HEADER_SIZE + ICMP_HEADER_SIZE));

            icmppacket(buf, size - (IP_HEADER_SIZE + ICMP_HEADER_SIZE),
                       saddr, status.ciaddr,
                       ICMP_ECHOREPLY, 0, status.macaddr);
            break;
        }

        // Ping response packet
        case ICMP_ECHOREPLY:
        {
            itc_printf("Ping reply received from %s\r\n", iptoa(saddr));
            break;
        }

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// icmpunreachable
// PURPOSE: Reply to a host with an ICMP protocol unreachable message
// PARAMS:  (IN)  u8 *dest  - buffer of IP packet triggering the reply
//          (IN)  u16 size  - size of packet.
//          (IN)  u8 code   - unreachable code
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
icmpunreachable(u8 * dest, u16 size, u8 code)
{
    u8 buf[ICMP_SIZE];
    u8 * const start = buf +
                       ETHER_HEADER_SIZE + IP_HEADER_SIZE + ICMP_HEADER_SIZE;
    u8 *curpos = start;
    const u16 len = 4 + IP_HEADER_SIZE + ICMP_DATAGRAM_COPY;
    u32 daddr, saddr;

    saddr = ntohl(((*(u16 *)(dest + IADDR_OFFSET + sizeof(u16))) << 16) |
                  (*(u16 *)(dest + IADDR_OFFSET)));
    daddr = ntohl(((*(u16 *)(dest + DADDR_OFFSET + sizeof(u16))) << 16) |
                  (*(u16 *)(dest + DADDR_OFFSET)));

    if (((saddr | status.smask) == IP_BROADCAST) ||
        ((daddr | status.smask) == IP_BROADCAST))
    {
        // Don't respond or send to broadcast addresses
        return;
    }

    *curpos++ = 0;              // unused
    *curpos++ = 0;              // unused
    *curpos++ = 0;              // unused
    *curpos++ = 0;              // unused

    // Copy the beginning of the packet which caused this error
    itc_memcpy(curpos, dest, IP_HEADER_SIZE + ICMP_DATAGRAM_COPY);

    icmppacket(buf, len, saddr, status.ciaddr,
               ICMP_DEST_UNREACH, code, status.macaddr);
}

////////////////////////////////////////////////////////////////////////////////
// icmpping
// PURPOSE: Ping a host with an ICMP Echo message
// PARAMS:  (IN)  u32 iaddr - address of machine to ping
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
icmpping(u32 iaddr)
{
    u8 buf[ICMP_SIZE];
    u8 * const start = buf +
                       ETHER_HEADER_SIZE + IP_HEADER_SIZE + ICMP_HEADER_SIZE;
    u8 *curpos = start;
    const u16 len = 4;

    *curpos++ = 0;              // identifier
    *curpos++ = 0;              // identifier
    *curpos++ = 0;              // sequence number
    *curpos++ = 0;              // sequence number

    icmppacket(buf, len, iaddr, status.ciaddr,
               ICMP_ECHO, 0, status.macaddr);
}
