//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      udp.c
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

#include <udp.h>
#include <ip.h>
#include <net.h>
#include <types.h>
#include <timer.h>

////////////////////////////////////////////////////////////////////////////////
// udppacket
// PURPOSE: Adds a UDP header to a packet, and send it over IP.
// IN: u16 *packet - Memory containing packet to send. Data starts 42 bytes in
//                  to leave space for the Ethernet header (14 bytes), IP
//                  header (20 bytes), and UDP header (8 bytes).
////////////////////////////////////////////////////////////////////////////////
void
udppacket(u16 *packet,
          u16 length,
          u32 daddr,
          u32 saddr,
          u16 dport,
          u16 sport,
          u16 *mymac)
{
   u16 *start = packet + ((ETHER_HEADER_SIZE + IP_HEADER_SIZE) / 
	        sizeof(u16)) - UDP_PSEUDO_HEADER_OFFSET;
   u16 *curpos = start;
   
   //scratch data needed for the udp checksum, will be overwritten by ippacket
   *curpos++ = htons(saddr >> 16);
   *curpos++ = htons(saddr & 0x0000FFFF);
   *curpos++ = htons(daddr >> 16);
   *curpos++ = htons(daddr & 0x0000FFFF);
   *curpos++ = htons(IPPROTO_UDP);
   *curpos++ = htons(length + UDP_HEADER_SIZE);

   //UDP header
   *curpos++ = htons(sport);
   *curpos++ = htons(dport);
   *curpos++ = htons(length + UDP_HEADER_SIZE);
   *curpos = 0;
   *curpos = ipsum((u8 *)(start), length + IP_HEADER_SIZE);

   ippacket((u8 *)packet, length + UDP_HEADER_SIZE, daddr, saddr, mymac,
            IPPROTO_UDP);
}

////////////////////////////////////////////////////////////////////////////////
// udplisten
// PURPOSE: Listens for a UDP packet on the network and retrieves the first one
//          it finds.
// PARAMS:  (IN)  u8 *dest   - buffer to hold the packet in.
//          (IN)  u16 *size  - size of buffer.
//          (OUT) u32 *iaddr - IP address the packet came from.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
int
udplisten(u8 *dest,
          u16 *size,
          u32 *iaddr,
	  char bcast_enable)
{
   const u16 isize = *size;
   u16 udplength;
   u8 protocol;

   do
   {
      *size = isize;
      if(!iplisten(dest, size, bcast_enable))
      {
         return 0;
      }

      protocol = *(dest + ETHER_HEADER_SIZE + IP_PROTOCOL_OFFSET);
      udplength = ntohs(*(u16 *)(dest + ETHER_HEADER_SIZE + IP_HEADER_SIZE +
                                 UDP_LENGTH_OFFSET));

      // TODO: should probably check UDP checksum here

   } while(protocol != IPPROTO_UDP ||
           (udplength + UDPIP_HEADER_SIZE - UDP_HEADER_SIZE) > *size);

   //this will not be 32 bit aligned
   *iaddr = (*(u16 *)(dest + ETHER_HEADER_SIZE + IADDR_OFFSET)) |
            ((*(u16 *)(dest + ETHER_HEADER_SIZE + IADDR_OFFSET + sizeof(u16)))
              << 16);

   return 1;
}
