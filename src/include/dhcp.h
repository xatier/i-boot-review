//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      dhcp.h
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
////////////////////////////////////////////////////////////////////////////////

#ifndef DHCP_H
#define DHCP_H

#include <types.h>

#define DHCP_PACKET_SIZE        (500)

#define DHCP_MAGIC1             (99)
#define DHCP_MAGIC2             (130)
#define DHCP_MAGIC3             (83)
#define DHCP_MAGIC4             (99)

#define DHCP_OPTION_PAD         (0)
#define DHCP_OPTION_SMASK	(1)
#define DHCP_OPTION_ROUTER	(3)
#define DHCP_OPTION_REQADDR     (50)
#define DHCP_OPTION_MSGTYPE     (53)
#define DHCP_OPTION_SERVER_ID	(54)
#define DHCP_OPTION_REQPARAM	(55)
#define DHCP_OPTION_END         (255)

#define DHCP_DISCOVER           (0x01)
#define DHCP_OFFER              (0x02)
#define DHCP_REQUEST            (0x03)
#define DHCP_DECLINE            (0x04)
#define DHCP_ACK                (0x05)
#define DHCP_NAK                (0x06)
#define DHCP_RELEASE            (0x07)
#define DHCP_INFORM             (0x08)

#define MAX_BOOTFILE_SIZE       (128)

typedef struct {
   u8 op;
   u8 htype;
   u8 hlen;
   u8 hops;
   u32 xid;
   u16 secs;
   u16 flags;
   u32 ciaddr;
   u32 yiaddr;
   u32 siaddr;
   u32 giaddr;
   u16 chaddr[8];
   char sname[64];
   char bootfile[128];
   u8 magic_cookie[4];
} __attribute__((packed)) dhcp_packet;

int init_dhcp(u32 *ciaddr, u32 *siaddr, u32 *giaddr, u32 *smask);
void parse_options_dhcp(u8 *end, u32 *giaddr, u32 *smask, u32 *sid,
                        u8 *msg_type);


#endif //DHCP_H
