//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ethernet.h
//
// Description:
//
//      Common Ethernet API.
//
// Author:
//
//      Dan Fandrich
//
// Created:
//
//      April 2002
//
////////////////////////////////////////////////////////////////////////////////

#ifndef ETHERNET_H
#define ETHERNET_H

#include <types.h>

#define ETHER_DEST_OFFSET		(0)
#define ETHER_SRC_OFFSET		(6)
#define ETHER_TYPE_OFFSET		(12)

#define ETHER_HABITLENGTH		(48)
#define ETHER_MIN_PACKET		(60)

int write_mac_ethernet(u16 *macaddr, unsigned short num);
int read_mac_ethernet(u16 *macaddr, unsigned short num);
int init_ethernet(u16 *macaddr);
int tx_packet_ethernet(u8 *data, u16 size);
int rx_packet_ethernet(u8 *data, u16 *size, char bcast_enable);
int read_eeprom_ethernet(u16 offset, u16 *value);
int write_eeprom_ethernet(u16 offset, u16 value);
int rx_ethernet_on(void);
int rx_ethernet_off(void);
int flush_ethernet(void);

#endif //ETHERNET_H
