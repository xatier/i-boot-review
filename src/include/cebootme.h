//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      cebootme.c
//
// Description:
//
//      Broadcasts a "bootme" packet to notify instances of MS Platform Builder
//      running on this subnet of our presence.
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

#ifndef CEBOOTME_H
#define CEBOOTME_H

#include <c_main.h>
#include <types.h>

#define CE_PROTOID		0x45444247
#define CE_SERVICE		0xFF
#define CE_FLAGS		0x01

#define CE_PB_PORT              (980)

typedef struct {
 u32 protoid;
 u8 service;
 u8 flags;
 u8 count;
 u8 cmd;
 u8 majorversion;
 u8 minorversion;
 u16 macaddr[3];
 u32 iaddr;
 char id[17];
 char name[17];
 u8 cpuid;
//#ifdef CE30
 u8 bootmeversion;
 u16 bootflags;
 u16 tftpport;
 u16 srvport;
//#endif
} __attribute__((packed)) bootme;

int cebootme(void);

#endif //CEBOOTME_H
