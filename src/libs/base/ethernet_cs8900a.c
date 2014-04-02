//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ethernet_cs8900a.c
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

//We want failures to get to the user
#define _DEBUG
#define _DEBUG_FAIL

#include <ethernet_cs8900a.h>
#include <ethernet.h>
#include <string.h>
#include <util.h>
#include <net.h>
#include <types.h>
#include <timer.h>
#include <c_main.h>
#include <messages.h>
#include <debug.h>

////////////////////////////////////////////////////////////////////////////////
// readcs
// PURPOSE: Reads data from a specified packetpage register on the CS89x0
// PARAMS:  (IN) u16 csreg - register to read from
// RETURNS:  u16 value of specified register
////////////////////////////////////////////////////////////////////////////////
inline static u16
readcs(u16 csreg)
{
   *(volatile u16 *)(BASE_ADDR + ADDRESS_PORT) = csreg;
   return (*(volatile u16 *)(BASE_ADDR + DATA_PORT));
}

////////////////////////////////////////////////////////////////////////////////
// writecs
// PURPOSE: Writes a u16 to a specified packetpage register on the CS89x0
// PARAMS:  (IN) u16 csreg - register to write to
//          (IN) u16 lhv - data to write to register
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
inline static void
writecs(u16 csreg,
        u16 lhv)
{
   *(volatile u16 *)(BASE_ADDR + ADDRESS_PORT) = csreg;
   *(volatile u16 *)(BASE_ADDR + DATA_PORT) = lhv;
}

////////////////////////////////////////////////////////////////////////////////
// notbusy
// PURPOSE: Wait for Crystal chip to stop being busy
// PARAMS:  None.
// RETURNS: None.
////////////////////////////////////////////////////////////////////////////////
static void
notbusy(void)
{
   while(readcs(SSR) & SSR_BUSY)
   {
      ; //do nothing
   }
}

////////////////////////////////////////////////////////////////////////////////
// probe_cs8900a
// PURPOSE: Look for signs of a CS8900A at the given address
// PARAMS:  (OUT) u16 * address to check
// RETURNS: 1 for success, 0 for failure.
// NOTE:    This function is safe to call even right after a reset
////////////////////////////////////////////////////////////////////////////////
static int
probe_cs8900a(char *address)
{
/*
   // The data sheet only says this is valid after a hard reset
   if(((*(volatile u16 *)(address + ADDRESS_PORT)) & SCAN_MASK) != SCAN_SIG)
   {
      return 0;
   }
*/
   // We are allowed to read this register even during a reset
   // (so says Cirrus AN205)
   if((readcs(SSR) & REG_NUMB_MASK) != 0x16)
   {
      return 0;
   }

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// toggle_sbhe
// PURPOSE: Toggle the /SBHE line on the CS8900
// PARAMS:  None.
// RETURNS: None.
// NOTES:   This is really ugly, as we have to access PCMCIA I/O space in order
//          to control A0, which is connected to /SBHE.
////////////////////////////////////////////////////////////////////////////////
static void
toggle_sbhe(void)
{
   u8 volatile temp;

   temp = *(u8 volatile *)(PCMCIA_SOCKET_0_IO_BASE + 1);
   temp = *(u8 volatile *)(PCMCIA_SOCKET_0_IO_BASE + 0);
   temp = *(u8 volatile *)(PCMCIA_SOCKET_0_IO_BASE + 1);
   temp = *(u8 volatile *)(PCMCIA_SOCKET_0_IO_BASE + 0);
}

////////////////////////////////////////////////////////////////////////////////
// init_ethernet
// PURPOSE: Initializes the cs8900a ethernet chip.
// PARAMS:  (OUT) u16 * to return the MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
init_ethernet(u16 *macaddr)
{
   u16 mac[3];

   DEBUG_4("Initializing CS8900\r\n");
   // Toggle /SBHE before we try probing the chip
   toggle_sbhe();

   // Look for signs of a CS8900A before we start doing things to it
   if (!probe_cs8900a((char *)BASE_ADDR))
   {
      error_print(CS_NOTFOUND_ERROR);
      DEBUG_1("(at address %x)\r\n", BASE_ADDR);
      return 0;
   }

   itc_printf("CS8900A found at 0x%x\r\n", BASE_ADDR);

   // Reset CS8900 chip
   writecs(SCR, readcs(SCR) | POWER_ON_RESET);

   // Delay for a very short time, then toggle /SBHE again after reset
   udelay(10);
   toggle_sbhe();

   DEBUG_4("Waiting for CS8900...");
   // Wait for chip to finish resetting (takes up to 10 ms)
   while(!(readcs(SSR) & SSR_INITD))
   {
      ; //do nothing
   }
   DEBUG_4("CS8900 reset\r\n");

   if(readcs(CS8900ID) != CS89x0_ISA_ID)
   {
      error_print(CS_ISAID_ERROR);
      return 0;
   }

   if((readcs(PROD_ID) & PROD_ID_MASK) != CS8900ID)
   {
      error_print(CS_NOTCS_ERROR);
      return 0;
   }

   if(((*(volatile u16 *)(BASE_ADDR + TX_CMD_PRT)) &
      REG_NUMB_MASK) != REG_NUMB_TX_CMD)
   {
      error_print(CS_NOTX_ERROR);
      return 0;
   }

   //check the Line Status Register to see if cabling is plugged in.
   // NOTE: this might not be reliable unless we wait a bit first
   if((readcs(LSR) & LSR_OK) == 0)
   {
      DEBUG_1("Link disconnected.\r\n");
   }
   else
   {
      DEBUG_2("Link connected\r\n");
   }

   if (!read_mac_ethernet(mac, 0))
   {
      error_print(CS_NOMAC_ERROR);
      return 0;
   }

   macaddr[0] = mac[0];
   macaddr[1] = mac[1];
   macaddr[2] = mac[2];

   //write MAC to the address reg
   writecs(IAR, mac[0]);
   writecs(IAR+2, mac[1]);
   writecs(IAR+4, mac[2]);

   //IO channel ready on
   writecs(BCR, (readcs(BCR) | BCR_IO_CHN_ON));

   //set Rx Config Register to defaults
   writecs(RX_CONFIG_REG, RX_DEFAULT);

   //set Tx Config Register to defaults
   writecs(TX_CONFIG_REG, TX_DEFAULT);

   //set Buffer Config Register to defaults
   writecs(BUFFER_CONFIG_REG, BUF_CONFIG_DEFAULT);

   //set up acceptance filter
   writecs(RCR, (RCR_RX_IA | RCR_RX_BROADCAST | RCR_RX_MULTICAST | RCR_RX_OK));

   //enable frame Rx and Tx
   writecs(LCR, (readcs(LCR) | LCR_SERIAL_RX_ON | LCR_SERIAL_TX_ON));

   writecs(TX_CMD_REQ, TX_AFTER_ALL | TX_PAD);
   writecs(TX_LENGTH, 0);
   while(!(readcs(BSR) & READY_FOR_TX_NOW));
   readcs(TX_EVENT);
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// tx_packet_ethernet
// PURPOSE: Transmits a packet.
// PARAMS:  (IN) u16 * to return the MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
tx_packet_ethernet(u8 *data,
                   u16 size)
{
   int i = 0;
   u16 temp = 0;
   u32 time = get_time_timer();

   //initiate tx sequence
   writecs(TX_CMD_REQ, TX_AFTER_ALL | TX_PAD);
   writecs(TX_LENGTH, size);

   //wait for the chip to free room in it's internal buffer
   while(!(readcs(BSR) & READY_FOR_TX_NOW))
   {
      if(time < get_time_timer() - NET_TIMEOUT)
      {
         return 0;
      }
   }

   //give the packet to the chip
   for(i = 0; i < (size / sizeof(u16));i++)
   {
      *(volatile u16 *)(BASE_ADDR + TX_FRAME_PORT) =  *(u16 *)data;
      data += sizeof(u16);
   }

   //if we are sending a packet of uneven size, send the last byte.
   if(size % sizeof(u16))
   {
      *(volatile u16 *)(BASE_ADDR + TX_FRAME_PORT) = *data;
   }

   //check for tx errors
   temp = readcs(TX_EVENT);
   if(temp & 
      (TX_EXCESSIVE_COL |
       TX_JABBER |
       TX_LOST_CRS |
       TX_OUT_OF_WINDOW |
       TX_SQE_ERROR))
   {
      return 0;
   }

   //if there are no errors, assume everything is ok
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// rx_packet_ethernet
// PURPOSE: Receives a packet.
// PARAMS:  (IN)  u8 * data  - buffer for received packet.
//          (IN)  u16 * size - size of buffer.
//          (OUT) u16 * size - actual size of received packet. If unchanged
//                             packet may be truncated.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
rx_packet_ethernet(u8 *data,
                   u16 *size,
                   char bcast_enable)
{
   u16 rx_size = 1;
   int i;
   u16 oddword;
   u16 temp;

   //see if the chip has a packet waiting to be copied out
   temp = readcs(RX_EVENT);
   if((temp & RX_PACKET_RECEIVED_OK) == 0)
   {
      return 0;
   }

   rx_size = readcs(RX_LENGTH);

   //tell the chip to automatically increment it's internal address register
   //each time we read from it.
   (*(volatile u16 *)(BASE_ADDR + ADDRESS_PORT)) = (RX_FRAME | AUTOINCREMENT);

   //copy the packet out of the chips buffer
   //if our destination address is u16 aligned, we can do 16 bit transfers
   if((u32)data % sizeof(u16))
   {
      for(i = 0;i < (rx_size / sizeof(u16));i++)
      {
         oddword = *(volatile u16 *)(BASE_ADDR + DATA_PORT);
         *(u8 *)data++ = (u8)oddword & 0xFF;
         *(u8 *)data++ = (u8)(oddword >> 8) & 0xFF;
      }
   }
   else
   {
      for(i = 0;i < (rx_size / 2);i++)
      {
         *(u16 *)data = *(volatile u16 *)(BASE_ADDR + DATA_PORT);
         data += 2;
      }
   }

   //If the rxed packet is of uneven size, we will need to get the last byte
   if(rx_size % sizeof(u16))
   {
      *data = (u8)(*(volatile u16 *)(BASE_ADDR + DATA_PORT) & 0xFF);
   }

   //For some reason, the CS8900 reports the size as one dword larger than it
   //actually is. Perhaps it is including the values of RX_EVENT and RX_LENGTH
   //in the size.
   *size = (int)rx_size - sizeof(u32);
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// read_mac_ethernet
// PURPOSE: Reads the MAC address.
// PARAMS:  (OUT) u16 * macaddr - An array to return the MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
read_mac_ethernet(u16 *macaddr, unsigned short num)
{
   if (num != 0)
   {
       // This driver supports only a single Ethernet chip
       return 0;
   }

   if (!probe_cs8900a((char *)BASE_ADDR))
   {
       // We don't seem to have a CS8900 to read
       return 0;
   }

   // If the reset configuration block of the EEPROM was read correctly
   // at reset time, then SSR_EEPROM_OK will be set.  Get the MAC address
   // from the CS8900 Individual Address Register in that case.
   if (readcs(SSR) & SSR_EEPROM_OK)
   {
       macaddr[0] = readcs(IAR+0);
       macaddr[1] = readcs(IAR+2);
       macaddr[2] = readcs(IAR+4);
       DEBUG_4("Got MAC from chip\r\n");
       return 1;
   }

   // The chip indicates that no reset configuration block was found.  This
   // means either that the MAC address is not set on this device, or was set
   // on an older version of I-Boot using our own nonstandard format.
   DEBUG_4("Got MAC from EEPROM\r\n");
   return (read_eeprom_ethernet(EEPROM_ADDR_IA_W0, &macaddr[0]) &&
           read_eeprom_ethernet(EEPROM_ADDR_IA_W1, &macaddr[1]) &&
           read_eeprom_ethernet(EEPROM_ADDR_IA_W2, &macaddr[2]));
}

////////////////////////////////////////////////////////////////////////////////
// write_mac_ethernet
// PURPOSE: Writes a new MAC address.
// PARAMS:  (IN) u16 * macaddr - An array containing the new MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
write_mac_ethernet(u16 *macaddr, unsigned short num)
{
   // See section 3.4.4 in the data sheet for the meaning of this structure
   static u16 data[]=
   {
      0xa120,

      //Block 1 (reg 0x20, len 2+1)
      0x2020,

      0x0300,
      0x0000,
      0x0000,

      //Block 2 (reg 0x2c, len 5+1)
      0x502c,

      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,
      0x0000,

      //Block 3 (reg 0x158, len 2+1)
      0x2158,

      0x0001, /* MAC ADDR */
      0x0203,
      0x0405,
   };

   int result = 1;
   unsigned int sum = 0;
   int i;

   /* insert MAC ADDR into block 3 */
   data[13] = macaddr[0];
   data[14] = macaddr[1];
   data[15] = macaddr[2];

   /* write configuration blocks */
   for(i=0; i < countof(data); i++)
   {
      sum += (data[i]&0xff) + (data[i]>>8);
      result &= write_eeprom_ethernet(i, data[i]);
   }
   sum &= 0xff;

   /* write the checksum */
   data[0] = (0x100 - sum);
   result &= write_eeprom_ethernet(i, data[0]);

   /*
    * write a copy of the MAC address here for backwards compatibility with
    * old bootloaders
    */
   result &= write_eeprom_ethernet(EEPROM_ADDR_IA_W0, macaddr[0]);
   result &= write_eeprom_ethernet(EEPROM_ADDR_IA_W1, macaddr[1]);
   result &= write_eeprom_ethernet(EEPROM_ADDR_IA_W2, macaddr[2]);

   return result;
}

////////////////////////////////////////////////////////////////////////////////
// read_eeprom_ethernet
// PURPOSE: Reads from the attached EEPROM.
// PARAMS:  (IN)  u16 offset  - Offset from which to read.
// PARAMS:  (OUT) u16 * value - Value read.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
read_eeprom_ethernet(u16 offset,
                     u16 *value)
{
   //wait for EEPROM to be free
   notbusy();

   writecs(ECR, (offset | EEPROM_READ_CMD));

   //wait for it to finish
   notbusy();

   *value = readcs(EEPROM_DATA);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// write_eeprom_ethernet
// PURPOSE: Writes to the attached EEPROM.
// PARAMS:  (IN) u16 offset - Offset from which to write.
//          (IN) u16 value - Value to write.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
write_eeprom_ethernet(u16 offset,
                      u16 value)
{
   //wait for EEPROM to be free
   notbusy();
   writecs(ECR, EEPROM_ERASE_ENABLE);
   notbusy();
   writecs(EEPROM_DATA, value);
   notbusy();
   writecs(ECR, (offset | EEPROM_WRITE_CMD));
   notbusy();
   writecs(ECR, EEPROM_ERASE_DISABLE);
   notbusy();

   return 1;
}

// Not strictly necessary on the Crystal
int rx_ethernet_off (void)
{
   return 1;
}

// Not strictly necessary on the Crystal
int rx_ethernet_on (void)
{
   return 1;
}

// Not strictly necessary on the Crystal
int flush_ethernet (void)
{
   return 1;
}
