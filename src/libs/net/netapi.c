//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      netapi.c
//
// Description:
//
//      API for network applications, reminiscent of Berkeley socket
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

#include <udp.h>
#include <net.h>
#include <icmp.h>
#include <util.h>
#include <c_main.h>
#include <netapi.h>
#include <debug.h>

/*
static inline int
is_printable(u8 c)
{
    return (c >= ' ' && c <= '~');
}

static void
hex_dump(u8 *data, int len)
{
    int i;
    for (i=0; i < len; ++i)
    {
        if (i % 16 == 0)
        {
            itc_printf("\r\n%x: ", i);
        }
        itc_printf("%d:%c, ", data[i], is_printable(data[i]) ? data[i]: '?');
    }
    itc_printf("\r\n");
}
*/

////////////////////////////////////////////////////////////////////////////////
// udp_recvfrom
// PURPOSE: Receive a UDP packet
// PARAMS:  (IN) netapiconn *conn - pointer to connection structure
//          (OUT) packet - buffer large enough to hold largest packet w/headers
//          (IN)  length - length of that buffer
//          (OUT) connfrom - if not NULL, the address & ports this packet
//                           came from
// RETURNS: number of bytes of data payload received, or -1 on error
////////////////////////////////////////////////////////////////////////////////
int
udp_recvfrom(netapiconn const * const conn, u8 * const packet, u16 length,
             netapiconn * const connfrom)
{
    u8 * const udp_header = packet + ETHER_HEADER_SIZE + IP_HEADER_SIZE;
    u32 iaddr;
    u16 dest_port;
    int udplength;

    if (!udplisten(packet, &length, &iaddr, 0))
    {
        return -1;      // Timeout
    }

    iaddr = ntohl(iaddr);
    dest_port = ntohs(*(u16 *)(udp_header + UDP_DEST_PORT_OFFSET));
    udplength = ntohs(*(u16 *)(udp_header + UDP_LENGTH_OFFSET)) - 
                UDP_HEADER_SIZE;

    DEBUG_4("Got packet: iaddr=%s, rxport=%i\r\n", iptoa(iaddr), dest_port);

    // ignore any tftp packets that come from IP addresses other than
    // the one we are interested in, or are on different ports.
    if((conn->ciaddr == iaddr || conn->ciaddr == 0) &&
       dest_port == conn->rxport)
    {

        //hex_dump(udp_header, ntohs(*(u16 *)(udp_header + UDP_LENGTH_OFFSET)));
        if (connfrom)
        {
           connfrom->rxport = dest_port;
           connfrom->txport = ntohs(*(u16 *)(udp_header + UDP_SRC_PORT_OFFSET));
           connfrom->ciaddr = iaddr;
        }
        return udplength;
    }

    // Notify the sender that we aren't interested in this packet
    icmpunreachable(packet + ETHER_HEADER_SIZE,
                    length - ETHER_HEADER_SIZE,
                    ICMP_PORT_UNREACH);
    return -1;  // Packet not for us
}

////////////////////////////////////////////////////////////////////////////////
// udp_bind
// PURPOSE: Prepare a netapiconn structure for receiving UDP packets
// PARAMS:  (IN) netapiconn *conn - pointer to connection structure
//          (IN) port - port on which to receive UDP packets, or 0 to choose
// RETURNS: none
////////////////////////////////////////////////////////////////////////////////
void
udp_bind(netapiconn *conn, u16 port)
{
   if (port == 0)
   {
       //we need a random port for the server to send to us
       port = rand();

       //but we'd prefer it be a non-privileged port
       if(port < IPPORT_RESERVED)
       {
          port += IPPORT_RESERVED;
       }
   }
   conn->rxport = port;
}


////////////////////////////////////////////////////////////////////////////////
// udp_connect
// PURPOSE: Prepare a netapiconn structure for sending UDP packets
// PARAMS:  (IN) netapiconn *conn - pointer to connection structure
//          (IN) addr - address to which to send UDP packets
//          (IN) port - port to which to send UDP packets
// RETURNS: none
////////////////////////////////////////////////////////////////////////////////
void
udp_connect(netapiconn *conn, u32 addr, u16 port)
{
   conn->ciaddr = addr;
   conn->txport = port;
}

////////////////////////////////////////////////////////////////////////////////
// udp_send
// PURPOSE: Sends a UDP packet
// PARAMS:  (IN) netapiconn *conn - pointer to connection structure
//
// RETURNS: none
////////////////////////////////////////////////////////////////////////////////
void
udp_send(netapiconn const * const conn, u8 *packet, u16 length)
{
   udppacket((u16 *)packet,
             length,
             conn->ciaddr,
             status.ciaddr,
             conn->txport,
             conn->rxport,
             status.macaddr);
}

