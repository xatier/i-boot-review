//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      tftp.c
//
// Description:
//
//      Handles tftp downloads and MS Platform Builder tftp uploads.
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

#ifndef TFTP_H
#define TFTP_H

#include <netapi.h>

// packet type
#define TFTP_OP_RRQ		(0x0001)
#define TFTP_OP_WRQ		(0x0002)
#define TFTP_OP_DATA		(0x0003)
#define TFTP_OP_ACK		(0x0004)
#define TFTP_OP_ERROR		(0x0005)

// payload size
#define TFTP_DATA_SIZE		(512)

// block number for ACK for WRQ packet
#define TFTP_ACK_WRQ		(0)

// byte offsets into various TFTP packets
#define TFTP_TYPE_OFFSET	(0)
#define TFTP_BLOCK_OFFSET	(2)
#define TFTP_ERROR_CODE_OFFSET  (2)
#define TFTP_ERROR_MSG_OFFSET   (4)

// size of TFTP header for TFTP_OP_DATA data packet
#define TFTP_HEADER_SIZE	(4)

// port to listen to in daemon mode
#define PB_PORT			(980)

// seconds between retries
#define TFTP_TIMEOUT_PERIOD     NET_TIMEOUT

// number of retries before giving up
#define TFTP_RETRIES            6

// well-known UDP port number for TFTP
#define IPPORT_TFTP 69

int tftplisten(u8 *dest);
void tftpack(u16 block, netapiconn conn);
int tftpget(char const *file, u8 *dest);

#endif //TFTP_H
