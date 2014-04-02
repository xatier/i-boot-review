////////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      arp.c
//
// Description:
//
//      Handles ARP requests and transmissions (RFC 826).
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      October 2001
//
// Modified:
// 	Brad Remedios - March 25, 2002
// 		- Added code to flush the ethernet controller when we know the
// 		  data is not valid and when we are done with it.  This helps
// 		  us in the busy network cases.
////////////////////////////////////////////////////////////////////////////////

#include <arp.h>
#include <ip.h>
#include <ethernet.h>
#include <types.h>
#include <c_main.h>
#include <timer.h>
#include <util.h>

u16 lastmac[3] = {0, 0, 0};
u32 lastiaddr = 0;

////////////////////////////////////////////////////////////////////////////////
// catchreply
// PURPOSE: Catches an arp reply destined for this host. Called after arp.
// PARAMS:  (OUT) u16 *macaddr - Ethernet address of remote host from arp reply.
//                               This value is undefined on failure.
//          (IN)  u32 iaddr    - The desired host's IP addr, in network order.
// RETURNS: int - 0 for failure (first packet was not an arp reply destined for
//                this host), 1 for success.
// NOTES:
// 	Arp is a Broadcasted protcol (response to ARP Request are sent as a 
// 	Broadcast) and thus the 1 in rx_packet_ethernet.
////////////////////////////////////////////////////////////////////////////////
inline static int
catchreply(u16 *macaddr,
           u32 iaddr)
{
   u8 buf[MAX_PACKET_SIZE];
   u8 *packet = buf;
   u16 size;
   arppacket *reply; 
   u32 targetiaddr = 0;
   u32 time = get_time_timer() + ARP_TIMEOUT;

   //we need the packet to be u32 aligned *after* the 42 byte header
   packet += ((u32)packet + ETHER_HEADER_SIZE) % sizeof(u32);
   reply = (arppacket *)(packet + ETHER_HEADER_SIZE);

   while (1)
   {
      size = MAX_PACKET_SIZE;
      while(!rx_packet_ethernet(packet, &size, 1))
      {
         if(time < get_time_timer())
            return 0;
      }

      // We're only interested in ARP packets right now
      if ((*(u16 *)(packet + ETHER_TYPE_OFFSET)) != ETHER_TYPE_ARP)
      {
         // TODO: this throws out all packets while waiting for a reply.
         // Normally, this happens quickly, but if the host doesn't exist,
         // there could be lots of pings, etc. that are lost. This should
         // be fixed one day.  Be careful to avoid endless recursion, though.
         continue;
      }

      // Drop short packets
      if ((size - ETHER_HEADER_SIZE) < sizeof(arppacket))
      {
         continue;
      }

      //only catch replies
      if (reply->opcode != ARP_REPLY)
      {
         continue;
      }

      //this probably won't be aligned.
      targetiaddr = ((reply->iaddr2 << 16) | (reply->iaddr1));

      //only catch packets from our target
      if (htonl(targetiaddr) != iaddr)
      {
         continue;
      }

      itc_memcpy((u8 *)macaddr, (u8 *)reply->macaddr, sizeof(u16) * 3);
      return 1;
   }
}

////////////////////////////////////////////////////////////////////////////////
// arp
// PURPOSE: Sends an arp request.
// PARAMS:  (IN) u32 iaddr     - IP address to send request for
//          (OUT) u16 *macaddr - Ethernet address from reply. Passed to
//                               catchreply.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
arp(u32 iaddr,
    u16 *macaddr)
{
   u8 buf[ARP_SIZE];
   u8 *packet = buf;
   arppacket *data = (arppacket *)(packet + ETHER_HEADER_SIZE);
   int i = 0;
   u16 broadcast[3] = {0xFFFF, 0xFFFF, 0xFFFF};
   int responded = 0;
   int timeout = 0;

   // Broadcast address
   // Network broadcast is not supported.
   if(iaddr == IP_BROADCAST)
   {
      memset16((u16 *)macaddr, 0xFFFF, 3);
      return 1;
   }
   // Get gateway's MAC instead of host's if host is on another subnet
   if((iaddr & status.smask) != (status.ciaddr & status.smask))
   {
      iaddr = status.giaddr;
   }
   // Try to satisfy the request from the cache
   if(iaddr == lastiaddr)
   {
      itc_memcpy((u8 *)macaddr, (u8 *)lastmac, sizeof(u16) * 3);
      return 1;
   }

   i = (u32)data % (sizeof(u32));
   packet += i;
   (u32)data += i;

   /*
    * We flush the ethernet controller because anything that is in there is
    * complete garbage.
    */
   flush_ethernet ();
  
   while(!responded && timeout++ < NET_TIMEOUT)
   {
      memset8(packet, 0, sizeof(arppacket));

      data->hdtype = ARP_HDTYPE;
      data->protocol = ARP_PROTOCOL;
      data->haddrln = (u8)(ETHER_HABITLENGTH / 8);
      data->protoln = (u8)(IP_ADDRBITLENGTH / 8);
      data->opcode = ARP_REQ;

      itc_memcpy((u8 *)data->macaddr, (u8 *)status.macaddr, sizeof(u16) * 3);
      memset8((u8 *)data->targetmac, 0, sizeof(u16) * 3);
      
      //this will not be 32 bit aligned
      data->iaddr1 = htons((status.ciaddr & 0xFFFF0000) >> 16);
      data->iaddr2 = htons((status.ciaddr & 0x0000FFFF));
      data->targetiaddr1 = htons((iaddr & 0xFFFF0000) >> 16);
      data->targetiaddr2 = htons(iaddr & 0x0000FFFF);

      etherpacket(packet,
                  sizeof(arppacket),
                  broadcast,
                  status.macaddr,
                  ETHER_TYPE_ARP);

      responded = catchreply(macaddr, iaddr);
   }

   /*
    * Since we are done with the ethernet controller lets turn on receive and
    * flush the internal memory so its nice and clean for the next function
    * to use.
    */
   rx_ethernet_off ();
   flush_ethernet ();

   if(timeout < NET_TIMEOUT)
   {
      // Store the results in the cache for next time
      lastiaddr = iaddr;
      itc_memcpy((u8 *)lastmac, (u8 *)macaddr, sizeof(u16) * 3);

      return 1;
   }
   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// arpreply
// PURPOSE: Handles an arp packet. Replies if it is directed at us.
// PARAMS:  (IN) u8 *packet - an arp packet
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
void
arpreply(u8 *packet, u16 size)
{
   arppacket *arppkt = (arppacket *)(packet + ETHER_HEADER_SIZE);
   u32 targetiaddr = (u32)((htons(arppkt->targetiaddr1) << 16) | 
                            htons(arppkt->targetiaddr2));

   // Drop short packets
   if ((size - ETHER_HEADER_SIZE) < sizeof(arppacket))
   {
      return;
   }

   //only reply to arp requests for this host
   if(targetiaddr != status.ciaddr)
   {
      return;
   }

   //only reply to requests
   if(arppkt->opcode != ARP_REQ)
   {
      return;
   }

   arppkt->targetiaddr1 = arppkt->iaddr1;
   arppkt->targetiaddr2 = arppkt->iaddr2;

   itc_memcpy((u8 *)arppkt->targetmac, (u8 *)arppkt->macaddr, sizeof(u16) * 3);
   arppkt->iaddr1 = htons((u16)(status.ciaddr >> 16));
   arppkt->iaddr2 = htons((u16)(status.ciaddr & 0x0000FFFF));
   itc_memcpy((u8 *)arppkt->macaddr, (u8 *)status.macaddr, sizeof(u16) * 3);
   arppkt->opcode = ARP_REPLY;

   etherpacket(packet,
               sizeof(arppacket),
               arppkt->targetmac,
               status.macaddr,
               ETHER_TYPE_ARP);
}
