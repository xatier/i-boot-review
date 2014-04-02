//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      bootp.h
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

#ifndef BOOTP_H
#define BOOTP_H

#define BOOTP_MAGIC1			(0x63)
#define BOOTP_MAGIC2			(0x82)
#define BOOTP_MAGIC3			(0x53)
#define BOOTP_MAGIC4			(0x63)
#define BOOTP_SUBMASK			(0x0401)
#define BOOTP_GATEWAY			(0x0403)
#define BOOTP_HOSTNAME			(0x200c)
#define BOOTP_ROOTDISK			(0x2011)
#define BOOTP_NIS			(0x2028)
#define BOOTP_EOF			(0x00FF)
#define BOOTP_REQ			(1)
#define BOOTP_REPLY			(2)
#define BOOTP_HATYPE_ETH		(1)
#define BOOTP_HALENGTH_ETH		(6)
#define BOOTP_CHADDR_OFFSET		(28)
#define BOOTP_VEND_OFFSET		(236)

#define BOOTP_PACKET_SIZE               (316)
#define BOOTP_TX_PORT                   (67)
#define BOOTP_RX_PORT                   (68)

int init_bootp(u32 *ciaddr,
               u32 *siaddr,
	       u32 *giaddr,
	       u32 *smask);

#endif //BOOTP_H
