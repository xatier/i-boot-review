//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      netapi.c
//
// Description:
//
//      API for network applications, reminiscent of Berkeley sockets
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

#ifndef NETAPI_H
#define NETAPI_H

typedef struct{
 u32 ciaddr;
 u16 rxport;
 u16 txport;
} netapiconn;

int udp_recvfrom(netapiconn const * const conn, u8 * const packet, u16 length,
             netapiconn * const connfrom);
void udp_bind(netapiconn *conn, u16 port);
void udp_connect(netapiconn *conn, u32 addr, u16 port);
void udp_send(netapiconn const * const conn, u8 *packet, u16 length);

#endif //NETAPI_H
