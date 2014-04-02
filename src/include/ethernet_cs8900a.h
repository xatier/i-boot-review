//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ethernet_cs8900a.h
//
// Description:
//
//      Driver for the Crystal CS8900a ethernet chip.
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

#ifndef ETHERNET_CS8900A_H
#define ETHERNET_CS8900A_H

#include <types.h>

#ifdef XSCALE
#define BASE_ADDR                       (u32)(0x04000300)
#define PCMCIA_SOCKET_0_IO_BASE         (u32)(0x20000000)
#else
#define BASE_ADDR                       (u32)(0x08000300)
#define PCMCIA_SOCKET_0_IO_BASE         (u32)(0x20000000)
#endif

#define AUTOINCREMENT			(u16)(0x8000)
#define CS89x0_ISA_ID			(u16)(0x630E)
#define PROD_ID				(u16)(0x0002)
#define PROD_ID_MASK			(u16)(0xE000)
#define CS8900ID			(u16)(0x0000)
#define ISQ_PORT                        (u16)(0x0120)
#define ADDRESS_PORT			(u16)(0x000A)
#define DATA_PORT			(u16)(0x000C)
#define SCAN_MASK			(u16)(0xF000)
#define SCAN_SIG			(u16)(0x3000)
#define TX_CMD_PRT			(u16)(0x0004)

#define REG_NUMB_MASK			(u16)(0x003F)
#define REG_NUMB_TX_CMD 		(u16)(0x0009)
#define LSR				(u16)(0x0134)  //Line Status Register
#define LSR_OK				(u16)(0x0080)
#define LCR				(u16)(0x0112)  //Line Control Register
#define LCR_SERIAL_RX_ON		(u16)(0x0040)
#define LCR_SERIAL_TX_ON		(u16)(0x0080)
#define SSR				(u16)(0x0136)  //Self Status Register
#define SSR_INITD			(u16)(0x0080)
#define SSR_BUSY			(u16)(0x0100)
#define SSR_EEPROM_OK			(u16)(0x0400)
#define ECR				(u16)(0x0040)  //EEPROM Command Register
#define EEPROM_READ_CMD			(u16)(0x0200)
#define EEPROM_WRITE_CMD		(u16)(0x0100)
#define EEPROM_ERASE_CMD		(u16)(0x0300)
#define EEPROM_ERASE_ENABLE		(u16)(0x00C0)
#define EEPROM_ERASE_DISABLE		(u16)(0x0000)
#define EEPROM_DATA			(u16)(0x0042)
#define EEPROM_ADDR_IA_W0		(u16)(0x001C)
#define EEPROM_ADDR_IA_W1		(u16)(0x001D)
#define EEPROM_ADDR_IA_W2		(u16)(0x001E)
#define IAR				(u16)(0x0158)  	//Individual Address Register
#define BCR				(u16)(0x0116)  	//Bus Control Register
#define BCR_IO_CHN_ON			(u16)(0x1000)
#define RCR				(u16)(0x0104)  	//Rx Control Register
#define RCR_RX_IA			(u16)(0x0400)
#define RCR_RX_BROADCAST		(u16)(0x0800)
#define RCR_RX_MULTICAST		(u16)(0x0200)
#define RCR_RX_OK			(u16)(0x0100)
#define RCR_RX_PROMISCUOUS		(u16)(0x0080)
#define RCR_RX_RUNT			(u16)(0x2000)
#define RCR_RX_EXTRA_DATA		(u16)(0x4000)
#define RCR_RX_BAD_CRC			(u16)(0x1000)
#define TCR				(u16)(0x0108)  //Tx Command Register
#define TX_AFTER_381			(u16)(0x0040)  //Tx after 381 bytes
#define TX_AFTER_ALL			(u16)(0x00C0)  //Tx after all bytes
#define TX_LENGTH			(u16)(0x0146)
#define BSR				(u16)(0x0138)  //Bus Status Register
#define READY_FOR_TX_NOW		(u16)(0x0100)
#define TX_FRAME_PORT			(u16)(0x0000)
#define RX_FRAME			(u16)(0x0404)
#define TX_EVENT			(u16)(0x0128)  //Tx Event Register
#define TX_EXCESSIVE_COL		(u16)(0x8000)
#define TX_JABBER			(u16)(0x0400)
#define TX_LOST_CRS			(u16)(0x0040)
#define TX_OUT_OF_WINDOW		(u16)(0x0200)
#define TX_SQE_ERROR			(u16)(0x0080)
#define TX_PACKET_OK			(u16)(0x0100)
#define TX_CMD_REQ			(u16)(0x0144)
#define TX_PAD				(u16)(0x0000)
#define RX_EVENT                        (u16)(0x0124)  //Rx Event Register
#define RX_STATUS			(u16)(0x0400)
#define RX_PACKET_RECEIVED_OK		(u16)(0x0100)
#define RX_LENGTH			(u16)(0x0402)
#define SCR				(u16)(0x0114)  //Self Control Register
#define SCR_HCO_1_ENABLE		(u16)(0x2000)
#define SCR_HCO_1			(u16)(0x8000)
#define RX_CONFIG_REG			(u16)(0x0102)  //Rx Config Register
#define RX_OK_ENABLE			(u16)(0x0100)
#define RX_CRC_ERROR_ENABLE		(u16)(0x1000)
#define RX_RUNT_ENABLE			(u16)(0x2000)
#define RX_EXTRA_DATA_ENABLE		(u16)(0x4000)
#define RX_BUFFER_CRC			(u16)(0x0800)
#define RX_DEFAULT			(u16)\
					(RX_OK_ENABLE |\
					RX_CRC_ERROR_ENABLE |\
					RX_RUNT_ENABLE |\
					RX_EXTRA_DATA_ENABLE |\
					RX_BUFFER_CRC)
#define TX_CONFIG_REG			(u16)(0x0106)  //Tx Config Register
#define TX_LOST_CRS_ENABLE		(u16)(0x0040)
#define TX_LATE_COLLISION_ENABLE	(u16)(0x0200)
#define TX_JABBER_ENABLE		(u16)(0x0400)
#define TX_16_COL_ENABLE		(u16)(0x8000)
#define TX_DEFAULT			(u16)\
					(TX_LOST_CRS_ENABLE |\
					TX_LATE_COLLISION_ENABLE |\
					TX_JABBER_ENABLE |\
					TX_16_COL_ENABLE)
#define BUFFER_CONFIG_REG		(u16)(0x010A)
#define TX_UNDERRUN_ENABLE		(u16)(0x0200)
#define TX_COL_COUNT_OVRFLOW_ENABLE	(u16)(0x1000)
#define RX_MISS_COUNT_OVRFLOW_ENABLE	(u16)(0x2000)
#define READY_FOR_TX_ENABLE		(u16)(0x0100)
#define BUF_CONFIG_DEFAULT		(u16)\
					(TX_UNDERRUN_ENABLE |\
					TX_COL_COUNT_OVRFLOW_ENABLE |\
					RX_MISS_COUNT_OVRFLOW_ENABLE |\
					READY_FOR_TX_ENABLE)
#define ENABLE_AUTO_INC			(u16)(0x8000)
#define POWER_ON_RESET			0x040 //bit 6 cause reset if we write it to SCR

#define MIN_FRAME_SIZE			64

#endif //ETHERNET_CS8900A_H
