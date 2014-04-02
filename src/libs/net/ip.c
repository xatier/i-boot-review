//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ip.c
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

#include <ip.h>
#include <arp.h>
#include <icmp.h>
#include <net.h>
#include <ethernet.h>
#include <timer.h>
#include <util.h>
//#define DEBUG_LEVEL 0
#include <debug.h>

////////////////////////////////////////////////////////////////////////////////
// ipsum
// PURPOSE: Calculates the UDP/IP header checksum.
// PARAMS:  (IN) char *addr - start of memory region to checksum.
//          (IN) int count  - number of bytes to checksum.
// RETURNS: u16 - checksum
////////////////////////////////////////////////////////////////////////////////
u16
ipsum(const char *addr,
      int count)
{
   u32 sum = 0;

   while(count > 1)
   {
      sum += *(u16 *)addr;
      addr += sizeof(u16);
      count -= sizeof(u16);
   }
 
   //Add left-over byte, if any
   if(count > 0)
   {
      sum += *(u8 *)addr;
   }

   //Fold 32-bit sum to 16 bits
   while (sum >> 16)
   {
      sum = (sum & 0xFFFF) + (sum >> 16);
   }
   return (u16)(~sum);
}

////////////////////////////////////////////////////////////////////////////////
// etherpacket
// PURPOSE: Adds an ethernet header to a packet, and transmits it.
// PARAMS:  (IN) u8 *packet - Packet to send. Data starts 14 bytes in, to leave
//                            room for the ethernet header.
//          (IN) u16 - length of the data in packet.
//          (IN) u16 *daddr - destination ethernet address
//          (IN) u16 *saddr - source ethernet address
//          (IN) u16 type - Ethernet packet type
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
etherpacket(u8 *packet,
            u16 length,
            u16 *daddr,
            u16 *saddr,
            u16 type)
{
   u8 *curpos = packet;

   //set destination address first
   itc_memcpy(curpos, (u8 *)daddr, sizeof(u16) * 3);
   curpos += sizeof(u16) * 3;

   //then source address
   itc_memcpy(curpos, (u8 *)saddr, sizeof(u16) * 3);
   curpos += sizeof(u16) * 3;

   //finally type field
   *(u16 *)curpos = type;
   curpos += sizeof(u16);

   tx_packet_ethernet(packet, length + ETHER_HEADER_SIZE);
}


////////////////////////////////////////////////////////////////////////////////
// ippacket
// PURPOSE: Adds an IP header to a packet, and sends it to etherpacket.
// PARAMS:  (IN) u8 *packet - packet to send
//          (IN) int length - length of data in packet
//          (IN) u32 daddr - destination IP address
//          (IN) u32 saddr - source IP address
//          (IN) u16 *mymac - ethernet address of this host
//          (IN) u8 protocol - IP protocol for this packet
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
ippacket(u8 *packet,
         int length,
         u32 daddr,
         u32 saddr,
         u16* mymac,
         u8 protocol)
{
   u8 *curpos = packet + ETHER_HEADER_SIZE;
   u16 mac[3];

   //generate the header
   *curpos++ = ((IP_VERSION << 4) | IP_HEADER_LENGTH);
   *curpos++ = IP_PRECEDENCE;
   *(u16 *)curpos = htons(length + IP_HEADER_SIZE);
   curpos += sizeof(u16);
   *(u16 *)curpos = htons(IP_ID);
   curpos += sizeof(u16);
   *(u16 *)curpos = htons((IP_CTR_FLAGS << 13) | (IP_OFFSET));
   curpos += sizeof(u16);
   *curpos++ = IP_TTL;
   *curpos++ = protocol;
   *(u16 *)curpos = IP_TEMP_CHECKSUM;
   curpos += sizeof(u16);
   *(u16 *)curpos = (u16)(((saddr & 0xFF000000) >> 24) |
                          ((saddr & 0x00FF0000) >> 8));
   curpos += sizeof(u16);
   *(u16 *)curpos = (u16)(((saddr & 0x0000FF00) >> 8) |
                          ((saddr & 0x000000FF) << 8));
   curpos += sizeof(u16);
   *(u16 *)curpos = (u16)(((daddr & 0xFF000000) >> 24) |
                          ((daddr & 0x00FF0000) >> 8));
   curpos += sizeof(u16);
   *(u16 *)curpos = (u16)(((daddr & 0x0000FF00) >> 8) |
                          ((daddr & 0x000000FF) << 8));
   curpos += sizeof(u16);
   //header generated

   //calculate and place the checksum
   *(u16 *)(packet + ETHER_HEADER_SIZE + IP_CHECKSUM_OFFSET) =
                               ipsum((char *)(packet + ETHER_HEADER_SIZE),
                                     IP_HEADER_SIZE);

   if(!arp(daddr, mac))
   {
      // Couldn't resolve the remote host
      return;
   }

   etherpacket(packet, (length + IP_HEADER_SIZE), mac, mymac, ETHER_TYPE_IP);
}

////////////////////////////////////////////////////////////////////////////////
// iplisten_poll
// PURPOSE: Listens for an IP packet.
// PARAMS:  (OUT) u8 *data - buffer to hold data
//          (OUT) u16 *size - size of packet received
//          (OUT) char bcast_enable - process broadcast packets when nonzero
// RETURNS: 1 for IP packet ready for processing, 0 for no IP packet returned.
// NOTES:   This function returns almost immediately.
////////////////////////////////////////////////////////////////////////////////
int
iplisten_poll(u8 *data,
              u16 *size,
	      char bcast_enable)
{
   int iplength = 0;
   u16 checksum = 0;
   u8 * const packet = data + ETHER_HEADER_SIZE;
   int ipsize;

   if (!rx_packet_ethernet(data, size, bcast_enable))
   {
      return 0;
   }
   ipsize = *size - ETHER_HEADER_SIZE;

   // Switch based on the Ethertype
   switch (*(u16 *)(data + ETHER_TYPE_OFFSET))
   {
       case ETHER_TYPE_ARP:
       {
          DEBUG_4("a");
          arpreply((u8 *)data, *size);
          return 0;
       }

       case ETHER_TYPE_IP:
       {
          DEBUG_4("i");
          break;
       }

       default:
       {
          // unknown protocol
          DEBUG_4("u");
          return 0;
       }
   }

   // Received an IP packet.
   iplength = ntohs(*(u16 *)(packet + IP_LENGTH_OFFSET));
   checksum = ipsum(packet, IP_HEADER_SIZE);

   // Use the packet length as found in the IP header as Ethernet can
   // pad packets to a longer length.
   *size = iplength + ETHER_HEADER_SIZE;

   // TODO: handle packets with IP options (maybe by copying the packet and
   // discarding the options)

   // Discard empty packets, non-IP packets, packets with a bad header
   // checksum, short packets, and IP packets containing options
   if (checksum != 0 || ipsize < iplength ||
       (*(packet + IP_HEADER_LENGTH_OFFSET) & 0xf) != IP_HEADER_LENGTH)
   {
      return 0;
   }

   // Switch based on the IP protocol type
   switch (*(packet + IP_PROTOCOL_OFFSET))
   {
        case IPPROTO_ICMP:
            DEBUG_4("C");
            icmpreply(packet, iplength);
            break;

        case IPPROTO_UDP:
            // We want to return UDP packets only from this function
            DEBUG_4("I");
            return 1;

        default:
            DEBUG_4("U");
            // Return an ICMP "protocol not available" message
            icmpunreachable(packet, iplength, ICMP_PROT_UNREACH);
            break;
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// iplisten
// PURPOSE: Listens for an IP packet.
// PARAMS:  (OUT) u8 *data - buffer to hold data
//          (OUT) u16 *size - size of packet received
//          (OUT) char bcast_enable - process broadcast packets when nonzero
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
iplisten(u8 *data,
         u16 *size,
	 char bcast_enable)
{
   const u16 isize = *size;
   u32 timeout = get_time_timer() + NET_TIMEOUT;

   DEBUG_4("w");
   *size = isize;
   while(!iplisten_poll(data, size, bcast_enable))
   {
      if(timeout < get_time_timer())
      {
         DEBUG_4("t");
         return 0;
      }
      *size = isize;
   }
   return 1;
}
