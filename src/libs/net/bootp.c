//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      bootp.c
//
// Description:
//
//      Sends bootp requests and handles the response.
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

#include <ethernet.h>
#include <util.h>
#include <c_main.h>
#include <ip.h>
#include <udp.h>
#include <bootp.h>
#include <types.h>
#include <timer.h>
#include <string.h>
#include <messages.h>
#include <dhcp.h>
#include <debug.h>

#define MAX_RETRIES 5

////////////////////////////////////////////////////////////////////////////////
// fillbootp
// PURPOSE: Fills in a bootp request.
// PARAMS:  (IN) u8 *start  - Start of memory to use. Assumes that enough space
//                            is available.
//          (IN) u16 *mymac - Ethernet address of this host. Assumed to be 6
//                            bytes long.
//          (IN) u32 stamp  - Unique identifier for this transaction
////////////////////////////////////////////////////////////////////////////////
static void
fillbootp(u8 *start,
          u16 const *mymac,
          u32 stamp)
{
   u8 *curpos = start;
   u16 secs = 0;

   memset8(start, 0, BOOTP_PACKET_SIZE);

   *curpos++ = BOOTP_REQ;
   *curpos++ = BOOTP_HATYPE_ETH;
   *curpos++ = BOOTP_HALENGTH_ETH;
   curpos++;

   //this might not be aligned
   *curpos++ = (stamp & 0xFF000000) >> 24;
   *curpos++ = (stamp & 0x00FF0000) >> 16;
   *curpos++ = (stamp & 0x0000FF00) >> 8;
   *curpos++ = (stamp & 0x000000FF);

   *(u16 *)curpos = secs;
   //skip unused portion aswell
   curpos += sizeof(u32);

   curpos = start + BOOTP_CHADDR_OFFSET;
   itc_memcpy(curpos, (u8 *)mymac, sizeof(u16) * 3);
   curpos += sizeof(u32) * 4;

   //skip the servername field
   curpos += 64;
   //skip the bootfile field
   curpos += 128;

   //set the magic cookie
   *curpos++ = DHCP_MAGIC1;
   *curpos++ = DHCP_MAGIC2;
   *curpos++ = DHCP_MAGIC3;
   *curpos++ = DHCP_MAGIC4;

   //specifically ask the DHCP server to tell us what the gateway address and
   //subnet mask are.
   *curpos++ = DHCP_OPTION_REQPARAM;
   *curpos++ = 2; //the number of parameters we are requesting
   *curpos++ = DHCP_OPTION_ROUTER;
   *curpos++ = DHCP_OPTION_SMASK;

   // End of options
   *curpos++ = DHCP_OPTION_END;
}

////////////////////////////////////////////////////////////////////////////////
// listenbootp
// PURPOSE: Listens for bootp response, returns status after one packet
//          received.
// PARAMS:  (IN)  u32 xid     - xid from the bootp request we sent.
//          (OUT) u32 *ciaddr - Clients (our) IP address.
//          (OUT) u32 *siaddr - Servers IP address.
// RETURNS: int - 0 for failure, 1 for success.
////////////////////////////////////////////////////////////////////////////////
static int
listenbootp(u32 xid,
            u32 *ciaddr,
            u32 *siaddr,
	    u32 *giaddr,
	    u32 *smask,
	    char *bootfile)
{
   u8 packet[MAX_PACKET_SIZE];
   u16 size = MAX_PACKET_SIZE;
   u8 *bootppacket = packet + UDPIP_HEADER_SIZE;
   u32 rxid;
   u32 iaddr;
   u32 time = get_time_timer() + NET_TIMEOUT;
   u32 client_iaddr;
   u32 server_iaddr;
   //scratch variables for parse_options_dhcp; we don't care about these
   u8 msg_type;
   u32 sid;

   while(1)
   {
      u8 bootp_type;

      if (time < get_time_timer())
      {
         // We timed out receiving reply
         // We can't rely on the udplisten timeout because we could
         // theoretically receive a steady supply of miscellaneous packets
         // so it will never time out.
         return 0;
      }

      if(udplisten(packet, &size, &iaddr, 1))
      {

         bootp_type = *bootppacket;
         bootppacket += 4;

         rxid = (*(bootppacket++) << 24);
         rxid |= (*(bootppacket++) << 16);
         rxid |= (*(bootppacket++) << 8);
         rxid |= *(bootppacket++);
         bootppacket += sizeof(u32) * 2;

         if(rxid == xid && bootp_type == BOOTP_REPLY)
         {
            client_iaddr = *bootppacket++ << 24;
            client_iaddr |= *bootppacket++ << 16;
            client_iaddr |= *bootppacket++ << 8;
            client_iaddr |= *bootppacket++;

            server_iaddr = *bootppacket++ << 24;
            server_iaddr |= *bootppacket++ << 16;
            server_iaddr |= *bootppacket++ << 8;
            server_iaddr |= *bootppacket++;

            break;
         }
      }
   }

   //skip the relay agent ip field
   bootppacket += sizeof(u32);
   //skip the hardware address field
   bootppacket += 16;
   //skip the servername field
   bootppacket += 64;
   //skip the bootfile field
   itc_strcpy(bootfile, (char *)bootppacket);
   DEBUG_3("Bootfile: %s\r\n", bootfile);
   bootppacket += 128;
   //skip the magic cookie
   bootppacket += sizeof(u32);

   parse_options_dhcp(bootppacket, giaddr, smask, &sid, &msg_type);

   *ciaddr = client_iaddr;
   *siaddr = server_iaddr;

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// init_bootp
// PURPOSE: Sends a bootp requests, and listens for response.
// PARAMS:  (IN) u32 *ciaddr - Client IP address returned.
//          (IN) u32 *siaddr - TFTP or other server address.
//          (IN) u32 *giaddr - gateway address.
//          (IN) u32 *smask  - IP netmask
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
init_bootp(u32 *ciaddr,
           u32 *siaddr,
	   u32 *giaddr,
	   u32 *smask)
{
   u8 bootpacket[BOOTP_PACKET_SIZE + UDPIP_HEADER_SIZE];
   int gotreply;
   int i = MAX_RETRIES;
   u32 stamp = rand();

   // Delay a short random amount to space out BOOTP requests from many clients
   // booting up at once.
   udelay(rand() % 100000);
   udelay(rand() % 100000);

   fillbootp(bootpacket + UDPIP_HEADER_SIZE, status.macaddr, stamp);

   flush_ethernet ();
   
   do
   {
      // Transmit BOOTP request
      udppacket((u16 *)bootpacket,
                BOOTP_PACKET_SIZE,
                IP_BROADCAST,
                0,                //we don't know what our IP address is yet
                BOOTP_TX_PORT,
                BOOTP_RX_PORT,
                status.macaddr);
      gotreply = listenbootp(stamp, ciaddr, siaddr, giaddr, smask,
			    (char *)&(status.bootfile));
      if (!gotreply)
      {
         itc_printf(".");
      }
   } while(!gotreply && i-- > 0);

   rx_ethernet_off ();
   flush_ethernet ();

   if(i <= 0)
   {
      itc_printf("\r\n");
      error_print(BOOTP_TIMEOUT_ERROR);
      return 0;
   }

   return 1;
}
