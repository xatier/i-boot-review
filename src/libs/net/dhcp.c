//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      dhcp.c
//
// Description:
//
//      Sends dhcp requests and handles the response.
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
// 		- Made the dhcp code make use of the Ethernet Flush and
// 		  related routines to help it out in the congested network
// 		  scenerios.  If the board gets too many broadcast packets
// 		  it may drop some on the floor.
////////////////////////////////////////////////////////////////////////////////

#include <dhcp.h>
#include <bootp.h>
#include <net.h>
#include <types.h>
#include <timer.h>
#include <c_main.h>
#include <udp.h>
#include <util.h>
#include <string.h>
#include <ethernet.h>
//#define DEBUG_LEVEL 4
#include <debug.h>

#define MAX_RETRIES 5   // number of times to try DHCP before giving up

////////////////////////////////////////////////////////////////////////////////
// fill_dhcp
// PURPOSE: Creates a dhcp request packet.
// PARAMS:  (OUT) u8 buf       - buffer to write packet into.
//          (IN)  u16 *macaddr - MAC address to create packet for.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
static void
fill_dhcp(dhcp_packet *packet,
         u16 *macaddr,
         u32 sid,
         u32 riaddr,
         u32 xid,
         u16 timestamp,
         u8 type)
{
   //we will need to fill in options at the end
   u8 *end = (u8 *)(packet + 1);

   //opcode
   packet->op = BOOTP_REQ;

   //hardware type
   packet->htype = BOOTP_HATYPE_ETH;

   //hardware address length
   packet->hlen = BOOTP_HALENGTH_ETH;

   //# of hops
   packet->hops = 0;

   //transaction id
   packet->xid = htonl(xid);

   //seconds since we started trying
   packet->secs = timestamp;

   //flags field (unused)
   packet->flags = 0;

   //client ip address (we don't know it yet)
   packet->ciaddr = 0;

   //"your" client ip address (set by server)
   packet->yiaddr = 0;

   //dhcp server address
   packet->siaddr = 0;

   //gateware ip address (set by server)
   packet->giaddr = 0;

   //client hardware address. 16 bytes long
   packet->chaddr[0] = macaddr[0];
   packet->chaddr[1] = macaddr[1];
   packet->chaddr[2] = macaddr[2];

   //server host name string. 64 bytes long
   packet->sname[0] = '\0';

   //boot file name (set by server). 128 bytes long
   packet->bootfile[0] = '\0';

   //set options "magic cookie"
   packet->magic_cookie[0] = DHCP_MAGIC1;
   packet->magic_cookie[1] = DHCP_MAGIC2;
   packet->magic_cookie[2] = DHCP_MAGIC3;
   packet->magic_cookie[3] = DHCP_MAGIC4;

   //Set DHCP message type
   *end++ = DHCP_OPTION_MSGTYPE;
   *end++ = sizeof(u8);
   *end++ = type;

   //pad to u32 alignment to make following u32 writes simpler
   *end++ = DHCP_OPTION_PAD;
   *end++ = DHCP_OPTION_PAD;
   *end++ = DHCP_OPTION_PAD;

   if(riaddr != 0)
   {
      *end++ = DHCP_OPTION_REQADDR;
      *end++ = sizeof(u32);
      *(u32 *)end = htonl(riaddr);
      end += sizeof(u32);

      //pad to u32 alignment to make following u32 write simpler
      *end++ = DHCP_OPTION_PAD;
      *end++ = DHCP_OPTION_PAD;
   }

   if(sid != 0)
   {
      *end++ = DHCP_OPTION_SERVER_ID;
      *end++ = sizeof(u32);
      *(u32 *)end = htonl(sid);
      end += sizeof(u32);
   }

   //specifically ask the DHCP server to tell us what the gateway address and
   //subnet mask are.
   *end++ = DHCP_OPTION_REQPARAM;
   *end++ = 3;  //the number of parameters we are requesting
   *end++ = DHCP_OPTION_ROUTER;
   *end++ = DHCP_OPTION_SMASK;
   *end++ = DHCP_OPTION_SERVER_ID;

   // End of options
   *end++ = DHCP_OPTION_END;
}

////////////////////////////////////////////////////////////////////////////////
// request_dhcp
// PURPOSE: Sends a DHCP request message.
// PARAMS:  (IN) u32 ciaddr - IP address to request
//          (IN) u32 sid    - DHCP server ID
//          (IN) u32 xid    - DHCP transaction id
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
static void
request_dhcp(u32 ciaddr,
             u32 sid,
             u32 xid)
{
   u8 buf[MAX_PACKET_SIZE + 2];
   //we want the packet to be u32 aligned *after* the headers (42 bytes)
   u8 *packet = buf + 2;

   memset32((u32 *)buf, 0 , MAX_PACKET_SIZE / sizeof(u32));

   fill_dhcp((dhcp_packet *)(packet + UDPIP_HEADER_SIZE),
             status.macaddr,
             sid,
             ciaddr,
             xid,
             0,
             DHCP_REQUEST);

   udppacket((u16 *)packet,
             DHCP_PACKET_SIZE + UDPIP_HEADER_SIZE,
             IP_BROADCAST,
             0,
             BOOTP_TX_PORT,
             BOOTP_RX_PORT,
             status.macaddr);
}

////////////////////////////////////////////////////////////////////////////////
// parse_options_dhcp
// PURPOSE: Parses the Options and vendor extentions field of DHCP and bootp
//          packets. Currently supports message type, subnet mask, and router
//          options.
// PARAMS:  (IN) u8 *end        - the end of the static part of the DHCP/bootp
//                                packet, and thus the beginning of the 
//                                extensions.
//          (OUT) u32 *giaddr   - Gateway (router) address
//          (OUT) u32 *smask    - Subnet mask
//          (OUT) u32 *sid      - Server ID
//          (OUT) u32 *msg_type - DHCP message type
// RETURNS: Nothing.
// NOTES:   The values of *giaddr, *smask, and *msg_type are undefined if they
//          are not found in the options field pointed to by end.
////////////////////////////////////////////////////////////////////////////////
void
parse_options_dhcp(u8 *end,
		   u32 *giaddr,
		   u32 *smask,
		   u32 *sid,
		   u8 *msg_type)
{
   int len;
   u8 *option;

   while(*end != DHCP_OPTION_END)
   {
      option = end++;
      len = *end++;
      DEBUG_3("Found DHCP option %d\r\n", *option);
      if(*option == DHCP_OPTION_MSGTYPE)
      {
	 *msg_type = *end;
      }
      else if(*option == DHCP_OPTION_SMASK)
      {
	 *smask = *end++ << 24;
	 *smask |= *end++ << 16;
	 *smask |= *end++ << 8;
	 *smask |= *end++;
	 DEBUG_2("DHCP reports subnet mask as %s\r\n", iptoa(*smask));
      }
      else if(*option == DHCP_OPTION_ROUTER)
      {
         *giaddr = *end++ << 24;
	 *giaddr |= *end++ << 16;
	 *giaddr |= *end++ << 8;
	 *giaddr |= *end++;
	 DEBUG_2("DHCP reports gateway at %s\r\n", iptoa(*giaddr));
      }
      else if(*option == DHCP_OPTION_SERVER_ID)
      {
	 *sid = *end++ << 24;
	 *sid |= *end++ << 16;
	 *sid |= *end++ << 8;
	 *sid |= *end++;
	 DEBUG_2("DHCP reports server ID as %s\r\n", iptoa(*sid));
      }
      else if(*option == DHCP_OPTION_PAD)
      {
         len = -1;      // special case
      }
      else
      {
	 DEBUG_3("Ignoring DHCP Option #%i.\r\n", *option);
      }
      end = option + len + 2;
   }
}

////////////////////////////////////////////////////////////////////////////////
// listen_dhcp
// PURPOSE: Listens for an incoming DHCP message for the board.
// PARAMS:  (OUT) u32 *ciaddr - IP address being assigned to the board.
//          (OUT) u32 *siaddr - DHCP server's IP address.
//          (OUT) u32 *giaddr - gateway address.
//          (OUT) u32 *smask  - IP netmask
//          (OUT) u32 *sid    - server ID
//          (OUT) char *bootfile - boot file name
//          (IN) u32 xid      - transaction ID
// RETURNS: Type of DHCP message received on success, 0 on failure.
////////////////////////////////////////////////////////////////////////////////
static u8
listen_dhcp(u32 *ciaddr,
            u32 *siaddr,
	    u32 *giaddr,
	    u32 *smask,
	    u32 *sid,
	    char *bootfile,
            u32 xid)
{
   u8 buf[MAX_PACKET_SIZE];
   //we need this to be u32 aligned *after* the 42 bytes of headers
   dhcp_packet *packet = (dhcp_packet *)(buf + UDPIP_HEADER_SIZE + 2);
   u32 iaddr;
   u16 size;
   u8 *end = (u8 *)((u32)packet + sizeof(dhcp_packet));
   u8 msg_type = 0;
   // Randomize the timeout a bit; his might overflow, but that would just
   // shorten the retry delay
   int time = get_time_timer() + NET_TIMEOUT + status.macaddr[2] % 4;

   do
   {
      if (time < get_time_timer())
      {
         // We timed out receiving reply
         // We can't rely on the udplisten timeout because we could
         // theoretically receive a steady supply of miscellaneous packets
         // so it will never time out.
         return 0;
      }

      size = MAX_PACKET_SIZE;
      //if udp times out, abandon our own timeout.
      if(!udplisten((buf + 2), &size, &iaddr,1))
      {
         return 0;
      }
   } while(packet->op != BOOTP_REPLY || ntohl(packet->xid) != xid ||
           size < (UDPIP_HEADER_SIZE + sizeof(dhcp_packet)));

   *ciaddr = ntohl(packet->yiaddr);
   *siaddr = ntohl(packet->siaddr);
   *giaddr = ntohl(packet->giaddr);

   itc_strcpy(bootfile, packet->bootfile);
   DEBUG_3("Bootfile: %s\r\n", bootfile);

   if (packet->magic_cookie[0] == DHCP_MAGIC1 &&
       packet->magic_cookie[1] == DHCP_MAGIC2 &&
       packet->magic_cookie[2] == DHCP_MAGIC3 &&
       packet->magic_cookie[3] == DHCP_MAGIC4)
   {
      parse_options_dhcp(end, giaddr, smask, sid, &msg_type);
   }

   DEBUG_3("Received a DHCP message of type: %i.\r\n", msg_type);
   return msg_type;
}

////////////////////////////////////////////////////////////////////////////////
// init_dhcp
// PURPOSE: Handles a DHCP transaction to obtain the board's IP address, a
//          default server IP address, and possibly other configuration details
//          in the future (such as a script to run).
// PARAMS:  (IN) u32 *ciaddr - Client IP address returned.
//          (IN) u32 *siaddr - TFTP or other server address.
//          (IN) u32 *giaddr - gateway address.
//          (IN) u32 *smask  - IP netmask
// RETURNS: 1 for success, 0 for failure. Parameters are not changed on failure.
//
// NOTES:
// 	The two labels (offer_wait_start and ack_wait_start) are there to help
// 	dhcp work well on very busy networks.  Also, Ethernet Flushes were
// 	added for the same reason
////////////////////////////////////////////////////////////////////////////////
int
init_dhcp(u32 *ciaddr,
          u32 *siaddr,
	  u32 *giaddr,
	  u32 *smask)
{
   u8 buf[MAX_PACKET_SIZE + 2];
   u32 xid = rand();
   u32 time = get_time_timer();
   //we want the packet to be u32 aligned *after* the headers (42 bytes)
   u8 *packet = buf + 2;
   int timeout = MAX_RETRIES;
   int got_addr = 0;

   // Delay a short random amount to space out DHCP requests from many clients
   // booting up at once.
   udelay(rand() % 100000);
   udelay(rand() % 100000);

   memset32((u32 *)buf, 0, MAX_PACKET_SIZE / sizeof(u32));
   fill_dhcp((dhcp_packet *)(packet + UDPIP_HEADER_SIZE),
            status.macaddr,
            0,  //we don't care which dhcp server responds to us
            0,  //we don't care what address it gives us
            xid,
            get_time_timer() - time,
            DHCP_DISCOVER);

   while(!got_addr && timeout--)
   {
      u32 temp_ciaddr = 0;
      u32 temp_siaddr = 0;
      u32 temp_giaddr = 0;
      u32 temp_smask = 0;
      u32 temp_sid = 0;
      int offer_retry;
      int ack_retry;
      char temp_bootfile[MAX_BOOTFILE_SIZE];

#if DEBUG_LEVEL >= 4
      DEBUG("Sending--------\r\n");
      parse_options_dhcp(packet + UDPIP_HEADER_SIZE + sizeof(dhcp_packet),
                         &temp_giaddr,
                         &temp_smask,
                         &temp_sid,
                         &temp_bootfile[0]);
      DEBUG("---------------\r\n");
#endif

      flush_ethernet ();
      udppacket((u16 *)packet,
                DHCP_PACKET_SIZE + UDPIP_HEADER_SIZE,
                IP_BROADCAST,
                0,             //we don't know what our ip is yet
                BOOTP_TX_PORT,
                BOOTP_RX_PORT,
                status.macaddr);

      offer_retry=0;
      if(listen_dhcp(&temp_ciaddr,
		     &temp_siaddr,
		     &temp_giaddr,
		     &temp_smask,
		     &temp_sid,
		     (char *)temp_bootfile,
		     xid) == DHCP_OFFER)
      {
         u32 ack_ciaddr = temp_ciaddr;
         u32 ack_siaddr = temp_siaddr;
	 u32 ack_giaddr = temp_giaddr;
	 u32 ack_smask = temp_smask;
         u32 ack_sid = temp_sid;
	 char ack_bootfile[MAX_BOOTFILE_SIZE];
         flush_ethernet ();
         request_dhcp(temp_ciaddr, temp_sid, xid);
	 ack_retry=0;
         do
         {
             if(listen_dhcp(&ack_ciaddr,
                            &ack_siaddr, 
                            &ack_giaddr,
                            &ack_smask,
                            &ack_sid, 
                            (char *)ack_bootfile,
                            xid) == DHCP_ACK)
             {
               *ciaddr = ack_ciaddr;
               *siaddr = ack_siaddr;
               *giaddr = ack_giaddr;
               *smask = ack_smask;
               itc_strcpy(status.bootfile, ack_bootfile);
               if(*siaddr == 0)
               {
                  // No more servers were given, so use this one as default
                  *siaddr = ack_sid;
               }
               if(*giaddr == 0)
               {
                  // No gateway was given, despite our request; use server
                  // address as a poor substitute.
                  *giaddr = *siaddr;
               }
               got_addr = 1;
               break;
            }
            itc_printf("+");
        } while (ack_retry++ < 3);
      }

      if (!got_addr)
      {
         itc_printf(".");
      }
   }

   rx_ethernet_off ();
   flush_ethernet ();

   if(!got_addr)
   {
      itc_printf("\r\n");
      error_print(DHCP_TIMEOUT_ERROR);
   }

   return got_addr;
}
