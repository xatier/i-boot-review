/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc
 *
 * FILE: ethernet_smsc91c111.c
 *
 * PURPOSE:
 * 	The purpose of this driver is to be the Ethernet driver for this
 * 	bootloader.
 *
 * REQUIRES:
 * 	24LC64 EEPROM Driver
 *
 * CREATED BY:
 * 	Brad Remedios
 *
 * SPECIAL NOTES:
 * 	The original Hardware that this was being used / tested on had a small
 * 	defect in that only 32-bit accesses would work.  Since then, that
 * 	defect has been fixed.  The appropriate way to access the SMSC
 * 	registers is the REG8/16/32.
 *
 * 	The hardware that this was initially used and tested on was slightly
 * 	broken.  All accesses to the SMSC have to be 32-bit aligned, even
 * 	though 8 and 16 bit accesses are totally valid.
 *
 * 	Additionally, the autoincrement functionality of the SMSC LAN91C111
 * 	was seen to work correctly only in the 32-bit accesses case.  If
 * 	the auto-increment is fixed, immediate performance increases can be
 * 	relized by modifing read_mmu_data and write_mmu_data as the
 * 	performance will increase in the unaligned cases (no performance
 * 	change for 32-bit aligned accesses).
 ****************************************************************************/

#include <string.h>
#include <ethernet_smsc91c111.h>
#include <ethernet.h>
#include <arp.h>
#include <timer.h>
#include <debug.h>

#ifdef TAGGED_EEPROM
#include <eeprom.h>
#include <eeprom_24lc64.h>
#else
#warning You are using obsolete EEPROM code
#include <eeprom_24lc64.h>
#endif

#define ERROR(x...) itc_printf("ERROR: "x)

#if DEBUG_LEVEL >= 3
// Print the Ethernet registers for debugging
static void
print_registers(void)
{
        int bank,reg;
        u16 _temp;

        itc_printf("91C111 registers:\r\n");
        for (bank=0; bank <= 3; ++bank)
        {
            SETUP_BANK(bank);
            itc_printf("%x-\r\n", bank);
            for (reg=0; reg < 0xe; reg += 2)
            {
                READ_REG16(reg, _temp);
                itc_printf("%x: %x\r\n", reg, _temp);
            }
            itc_printf("\r\n");
        }
}

// Print the phy registers for debugging
static void
print_phy_registers(void)
{
        int reg;
        u16 _temp;

        itc_printf("PHY registers:\r\n");
        for (reg=0; reg < 5; ++reg)
        {
            read_phy_register (reg, &_temp);
            itc_printf("%x: %x\r\n", reg, _temp);
        }
        itc_printf("\r\n");
}
#endif

/* 
 * These are debugging calls and aren't needed in normal operation
 *
static int print_data (unsigned char *data, unsigned short size);
 */
int init_ethernet(u16 *macaddr)
{
	int retval;
	unsigned short phy_data;
	unsigned short _temp;
	int counter;

	/*
	 * Can we find a SMSC 91C111?
	 */
	retval = probe_smsc91c111 ((unsigned int *)SMSC91C111_WINDOW_ADDR);

#if DEBUG_LEVEL >= 4
        print_registers();
        print_phy_registers();
#endif

	if (retval == 0)
	{
		ERROR("No SMSC LAN91C111 was detected at %x\r\n",
		      SMSC91C111_WINDOW_ADDR);
		return 0;
	}

	/*
	 * Reset the SMSC Ethernet Chip MAC.  Cases like Watchdog or Software
	 * Reset could leave it in a bad state otherwise.
	 */
	SETUP_BANK(BANK_RCR);
	WRITE_REG16(OFFSET_RCR, BIT_RCR_SOFTRST);
	WRITE_REG16(OFFSET_RCR, 0);

	SETUP_BANK(BANK_CNFGR);
	WRITE_REG16(OFFSET_CNFGR, BIT_CNFGR_EPH_PWR_EN);

	/*
	 * Promiscuous Mode - This is only here for testing.
	 */
/*
	SETUP_BANK(BANK_RCR);
	READ_REG16(OFFSET_RCR, _temp);
	_temp |= BIT_RCR_PRMS;
	WRITE_REG16(OFFSET_RCR, _temp);
*/	
	/*
	 * Turn off the transmitter and set the TCR to a known state
	 */
	SETUP_BANK(BANK_TCR);
	WRITE_REG16(OFFSET_TCR, 0);
	
	/*
	 * Automatically deallocate memory for TX packets so we don't have to
	 * NOTE: if this is enabled, then the 91C111 doesn't generate
	 * BIT_ISR_TX_INT, which is undocumented behaviour and messes up
	 * the TX completion check in tx_packet_ethernet.
	 */
/*
	SETUP_BANK(BANK_CR);
	READ_REG16(OFFSET_CR, _temp);
	_temp |= BIT_CR_AUTO_RELEASE;
	WRITE_REG16(OFFSET_CR, _temp);
*/

	/*
	 * Busy wait for MMU to become free, then Reset the MMU.
	 */
        flush_ethernet();

	/*
	 * Disable All Interrupts.  IBoot has no knowledge of them
	 * so we can't use them
	 */
	SETUP_BANK(BANK_IMR);
	WRITE_REG16(OFFSET_IMR, 0);

	/*
	 * We set up the Receiver to Strip CRC's off but DO NOT turn on
	 * the Receive Enable.  Because we poll the card, the SMSC's Memory
	 * will fill up quickly if we turn on RX Enable (only can store 4
	 * packets and will read broadcast.)
	 * Enable collision abort so we don't get half packets.
	 */
	SETUP_BANK(BANK_RCR);
	REG16(OFFSET_RCR) |= (BIT_RCR_STRIP_CRC | BIT_RCR_ABORT_ENB);

	/*
	 * Enable TXEN so that we may send out packets
	 */
	SETUP_BANK(BANK_TCR);
	READ_REG16(OFFSET_TCR, _temp);

#ifdef FULL_DUPLEX
	_temp |= BIT_TCR_TXENA | BIT_TCR_SWFDUP | BIT_TCR_PAD_EN;
#else
	_temp |= BIT_TCR_TXENA | BIT_TCR_PAD_EN;
#endif
	WRITE_REG16(OFFSET_TCR, _temp);
	
	/*
	 * Reset the PHY and wait for it to complete.
	 */
	SETUP_BANK(BANK_MI);
	write_phy_register (PHY_CR, PHY_CR_RST);
	
	counter = PHY_RESET_WAIT;
	do
	{
		read_phy_register (PHY_CR, &phy_data);
	} while ((phy_data & PHY_CR_RST) && --counter);

	if (!counter)
	{
		ERROR ("PHY Failed to Reset\r\n");
	}

#ifdef SECOND_ETHERNET
	if ((retval = read_mac_ethernet (macaddr,1)) == 0)
#else
	if ((retval = read_mac_ethernet (macaddr,0)) == 0)
#endif
	{
		ERROR("Unable to read MAC address\r\n");
		return retval;
	}

	/*
	 * Sometimes (~1 in 6 times) the PHY appears to ignore the first
	 * write after a reset, so do a sacrificial write by writing the
	 * default values.
	 */
	write_phy_register (PHY_CR,
	                    PHY_CR_SPEED | PHY_CR_ANEG_EN | PHY_CR_MII_DIS);

	/*
	 * Setup MAC Address in SMSC LAN91C111
	 */
  	SETUP_BANK(BANK_IAR);
	WRITE_REG16(OFFSET_IAR01, macaddr[0]);
	WRITE_REG16(OFFSET_IAR23, macaddr[1]);
	WRITE_REG16(OFFSET_IAR45, macaddr[2]);

	/*
	 * Setup Transmit Protocol Options:
	 * 	Auto Negotiation Off, LEDA = TX/RX, LEDB = 100MBit Link
	 */
	SETUP_BANK(BANK_RPCR);
        _temp = ((RPCR_TX_RX_LED << 5) | (RPCR_LINK_LED << 2)
#ifdef FAST_ETHERNET
                 | BIT_RPCR_SPEED
#endif

#ifdef FULL_DUPLEX
                 | BIT_RPCR_DPLX
#endif
                );
	WRITE_REG16(OFFSET_RPCR, _temp);

	/*
	 * Turn off AutoNegotiation.  Bring PHY out of isolation mode and
	 * set to 100MBit/s half Duplex
	 */
	write_phy_register (PHY_CR, 0
#ifdef FULL_DUPLEX
	                    | PHY_CR_DPLX
#endif

#ifdef FAST_ETHERNET
                            | PHY_CR_SPEED
#endif
	);

	itc_printf (
#ifdef FAST_ETHERNET
	"100 Mbps "
#else
	"10 Mbps "
#endif

#ifdef FULL_DUPLEX
        "full"
#else
        "half"
#endif
	" duplex mode\r\n");

	// We need to clear stale data, some states retain till read
	// and the link doesn't seem to be detected until we do.
	read_phy_register (PHY_SR, &_temp);

	/*
	 * It takes upwards of a second for the chip to detect whether a link
	 * is available.  Problems ensue if you try to use the link
	 * before it's detected, so we'd better wait.
	 */
	delay(2);

	// We need to clear stale data, some states retain till read
	read_phy_register (PHY_SR, &_temp);
	read_phy_register (PHY_SR, &_temp);
	
	if (_temp & PHY_SR_LINK)
	{
		itc_printf ("Ethernet Link Detected\r\n");
	} else
	{
		itc_printf ("No Ethernet Link Detected\r\n");
	}

#if DEBUG_LEVEL >= 3
        print_registers();
        print_phy_registers();
#endif

	return 1;
}  // End of init_ethernet

int tx_packet_ethernet(u8 *data, u16 size)
{
	unsigned char packet_number;
	unsigned int _temp;
	unsigned char _temp8;
	int retry=0,counter;

        DEBUG_4 ("Tx...");
retransmit:

	/*
	 * Both RXEN and TXEN should be on, I will double check though
	 */
	SETUP_BANK(BANK_TCR);
	if (!(REG16(OFFSET_TCR) & BIT_TCR_TXENA))
	{
		REG16(OFFSET_TCR) |= BIT_TCR_TXENA;
	}

	SETUP_BANK(BANK_RCR);
	if (!(REG16(OFFSET_RCR) & BIT_RCR_RXEN))
	{
                //ERROR ("RXEN was off\r\n");
		REG16(OFFSET_RCR) |= BIT_RCR_RXEN;
	}

#if DEBUG_LEVEL >= 5
        SETUP_BANK(BANK_ISR);
        READ_REG8(OFFSET_ISR, _temp8);
        if (_temp8 & BIT_ISR_TX_INT)
        {
                ERROR("Hey--TX_INT is set! %x\r\n", _temp8);
                // Clear the TX interrupt bit
                //SETUP_BANK (BANK_ISR);
                //WRITE_REG8 (OFFSET_IRQAR, BIT_ISR_TX_INT);
        }
#endif

	/*
	 * Try to Allocate Memory for a packet in the MMU.  If this fails
	 * we cannot continue.  
	 *
	 * If this fails we most likely have no more memory left.
	 */
	if (!allocate_tx (&packet_number))
	{
		ERROR("%s: Could not allocate packet for TX\r\n",
				__FUNCTION__);
		return 0;
	}

	SETUP_BANK(BANK_PNR);
	WRITE_REG8(OFFSET_PNR, packet_number);

	if (!write_mmu_data (packet_number, data, size))
	{
		ERROR("%s: Error Writing Data to MMU\r\n", __FUNCTION__);
		return 0;
	}

	SETUP_BANK(BANK_MMUCR);
	WRITE_REG16(OFFSET_MMUCR, OPCODE_MMUCR_ENQUEUE_TX);

#if DEBUG_LEVEL >= 5
        SETUP_BANK(BANK_ISR);
        READ_REG8(OFFSET_ISR, _temp8);
        if (_temp8 & BIT_ISR_TX_INT)
        {
		ERROR("TX_INT is already set! %x\r\n", _temp8);
                // Clear the TX interrupt bit
                //SETUP_BANK (BANK_ISR);
                //WRITE_REG8 (OFFSET_IRQAR, BIT_ISR_TX_INT);
        }
#endif

	/*
	 * Wait for packet to be done transmitting
	 * This is not possible if BIT_CR_AUTO_RELEASE is set.
	 */
        SETUP_BANK(BANK_ISR);
        counter=MAX_ISR_COUNTER;
        do
        {
                READ_REG8(OFFSET_ISR, _temp8);
        } while	(!(_temp8 & BIT_ISR_TX_INT) && --counter);

        if (!counter)
        {
                SETUP_BANK(BANK_EPHSR);
                READ_REG16(OFFSET_EPHSR, _temp);
                ERROR ("Timeout on TX\r\n");
                DEBUG_2("EPHSR %x\r\n", _temp);
                if (_temp & BIT_EPHSR_TXUNRUN)
                {
                        ERROR ("Tx Under Run Occurred\r\n");
                }
                DEBUG_2("ISR is %x\r\n", _temp8);

                if (!(_temp & BIT_EPHSR_TX_SUC))
                        ERROR ("No Tx Success\r\n");

                SETUP_BANK(BANK_TCR);
                READ_REG16(OFFSET_TCR, _temp);
                if (!(_temp & BIT_TCR_TXENA))
                {
                        ERROR ("Transmit Enabled not set\r\n");
                }

                return 0;
        }

#if DEBUG_LEVEL >= 5
        /*
         * For some reason we must read OFFSET_FPR here or else 
         * OFFSET_EPHSR doesn't always set BIT_EPHSR_TX_SUC below
         * when BIT_CR_AUTO_RELEASE is being used
         */
        SETUP_BANK(BANK_FPR);
        READ_REG16(OFFSET_FPR, _temp);

        /*
         * Do a sanity check on the packet number.
         * Not reliable when we are using BIT_CR_AUTO_RELEASE
         */
        if (!(_temp & BIT_FPR_TEMPTY) &&
            (_temp & MASK_FPR_PACKET_NUM) != packet_number)
        {
                ERROR ("Packet # mismatch %d,%d\r\n",
                       _temp & MASK_FPR_PACKET_NUM, packet_number);
        }
#endif

        /*
         * Check transmission status.  It might be better to check
         * it from the TX Packet itself if we intended to queue up a
         * bunch, send and so on.  (Currently, a tx packet must finish
         * before another is queued.)
         */
        SETUP_BANK(BANK_EPHSR);
        READ_REG16(OFFSET_EPHSR, _temp);
	if (_temp & BIT_EPHSR_TX_SUC)
	{
                DEBUG_4 ("Tx OK\r\n");

		// Release memory here because BIT_CR_AUTO_RELEASE is unset
		release_tx_packet (packet_number);

                // Clear the TX interrupt bit
                SETUP_BANK (BANK_ISR);
                WRITE_REG8 (OFFSET_IRQAR, BIT_ISR_TX_INT);

		/*
		 * Enable Receive
		 */
		SETUP_BANK(BANK_RCR);
		READ_REG16(OFFSET_RCR, _temp);
		_temp |= BIT_RCR_RXEN;
		WRITE_REG16(OFFSET_RCR, _temp);

		return 1;
	}	
	else 
	{
                DEBUG_2 ("Tx unsuccessful %x\r\n", _temp);
                DEBUG_2 ("ISR is %x\r\n", _temp8);
		release_tx_packet (packet_number);

                // Clear the TX interrupt bit
                SETUP_BANK (BANK_ISR);
                WRITE_REG8 (OFFSET_IRQAR, BIT_ISR_TX_INT);

                if (retry++ < MAX_RETRANSMIT)
                {
                    DEBUG_2 ("Retry Tx\r\n");
                    goto retransmit;
		}
                else
                {
                    return 0;
		}
	}

	return 1;
} // End of tx_packet_ethernet


int rx_packet_ethernet(u8 *data, u16 *size, char bcast_enable)
{
	unsigned char packet_number;
	unsigned short _temp;
	const unsigned short start_size=*size;
	int retry=0;
	int success;

	/*
	 * Enable the SMSC to Receive Packets if it is not already able to
	 * do so.
	 */
	SETUP_BANK(BANK_RCR);
	if (!(REG16(OFFSET_RCR) & BIT_RCR_RXEN))
	{	
		REG16(OFFSET_RCR) |= BIT_RCR_RXEN;
	}

restart_rx:
	/*
	 * Wait for the RX Packet to Come in
	 */
	SETUP_BANK(BANK_ISR);
	if (!(REG8(OFFSET_ISR) & BIT_ISR_RCV_INT))
	{
		return 0;
	}

	/*
	 * Double check that we have Received Packets Waiting
	 * This doesn't seem necessary with the BIT_ISR_RCV_INT check above,
	 * but we need the packet number from this register regardless.
	 */
	SETUP_BANK(BANK_FPR);
	READ_REG16(OFFSET_FPR, _temp);
	if (_temp & BIT_FPR_REMPTY)
	{
		return 0;
	}

        DEBUG_4 ("Rx...");
	/*
	 * Grab the Packet Number out of the Fifo Ports Register
	 */
	packet_number = _temp >> 8;
	
	*size = start_size;
	if (!(success = read_mmu_data (&packet_number, data, size)))
	{
		DEBUG_2 ("%s: read_mmu_data Failed\r\n", __FUNCTION__);
	}

	if (!release_rx_packet ())
	{
		ERROR ("%s: Failed to Release Received Packet\r\n",
				__FUNCTION__);
	}

	if (!success)
	{
		return 0;
	}

	/*
	 * To allow this to work better, we send it whether we want broadcast
	 * packets or not.  ARP and DHCP both need Broadcast, Bootp and tftp
	 * don't.  This will make bootp really, really quick and same with
	 * tftp.  It also makes tftp quick because it drops the packets on
	 * the floor.  Remember that we must honour ARP requests at all times
	 * because the host's ARP cache might be invalidated at any time.
	 *
	 * If we want even more performance, we should do this in 
	 * read_mmu_data however, that is not currently done that way because
	 * it is not clean.
	 */
	if (!bcast_enable)
	{
		if ((data[0]== 0xFF) && (data[1] == 0xFF) &&
		    (data[2] == 0xFF) && (data[3]== 0xFF) &&
		    (data[4] == 0xFF) && (data[5] == 0xFF) &&
                    (data[ETHER_TYPE_OFFSET] != (ETHER_TYPE_ARP & 0xff) ||
                     data[ETHER_TYPE_OFFSET+1] != ETHER_TYPE_ARP >> 8))
		{
                        DEBUG_3 ("Dropping b'cast RX\r\n");

			if (retry++ > MAX_RX_RETRY)
			{
				return 0;
			}
			else
			{
				goto restart_rx;
			}
		}
	}

        // There seems to be a bug somewhere that causes the occasional 
        // packet we transmit to show up here as an incoming packet.
        // Until we figure out why, detect those packets and throw them away.
        SETUP_BANK(BANK_IAR);
        if ((REG16(OFFSET_IAR01) == *(u16 *)(data + ETHER_SRC_OFFSET)) &&
            (REG16(OFFSET_IAR23) == *(u16 *)(data + ETHER_SRC_OFFSET + 2)) &&
            (REG16(OFFSET_IAR45) == *(u16 *)(data + ETHER_SRC_OFFSET + 4)))
        {
            DEBUG_2("Ignoring bogus received packet from myself\r\n");
            return 0;
        }

        DEBUG_4 ("Rx OK\r\n");
	return 1;
} // End of rx_packet_ethernet

////////////////////////////////////////////////////////////////////////////////
// read_mac_ethernet
// PURPOSE: Reads the MAC address.
// PARAMS:  (OUT) u16 * macaddr - An array to return the MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int read_mac_ethernet(u16 *macaddr, unsigned short num)
{
    u16 mac_size=sizeof(short) * 3;
#ifdef TAGGED_EEPROM
    char tag[7];

    // Try to read the tagged format MAC address.  If that fails (likely
    // because the EEPROM isn't in tagged format) fall back to the old format.
    if (check_eeprom_header())
    {
        itc_strcpy(tag, "MACADx");
        tag[5] = num + '0';
        if (eeprom_get_item(tag, sizeof(tag)-1, (u8 *)macaddr, &mac_size) <= 0)
        {
            DEBUG_3("Could not find EEPROM tag %s\r\n", tag);
            return 0;
        }
        return 1;
    }
    // If the EEPROM isn't in tagged format yet, fall through to old-style
    // reading routine (this is needed in order to read old addresses in
    // order to upgrade the EEPROM format).
#endif

    // Read the MAC address from the old-style fixed location in EEPROM
    // THIS IS OBSOLETE but needed for reading an old-style MAC address
    return read_data_eeprom (EEPROM_MAC_ADDR + (num * mac_size), 
                             (unsigned char*)macaddr, &mac_size);
} // End of read_mac_ethernet

////////////////////////////////////////////////////////////////////////////////
// write_mac_ethernet
// PURPOSE: Writes a new MAC address.
// PARAMS:  (IN) u16 * macaddr - An array containing the new MAC address.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int write_mac_ethernet(u16 *macaddr, unsigned short num)
{
    u16 mac_size=sizeof(short) * 3;
#ifdef TAGGED_EEPROM
    char tag[7];
    itc_strcpy(tag, "MACADx");
    tag[5] = num + '0';
    if (eeprom_write_item(tag, sizeof(tag)-1, (u8 *)macaddr, mac_size) < 0)
    {
        ERROR("Could not write to EEPROM\r\n");
        return 0;
    }
    return 1;

#else
    // Write the MAC address into the old-style fixed location in EEPROM
    // THIS IS OBSOLETE!
    return write_data_eeprom (EEPROM_MAC_ADDR + (num * mac_size),
                              (unsigned char*)macaddr,	&mac_size);
#endif
} // End of write_mac_ethernet

/*
 * Here starts local functions
 */
static int probe_smsc91c111 (unsigned int *address)
{
	unsigned short data;

	SETUP_BANK(BANK_BSR);
	READ_REG16(OFFSET_BSR, data);
	
	if ((data & 0xff00) == BSR_UNIQUE)
	{
		itc_printf ("SMSC LAN91C111 Found at Address 0x%x\r\n",
				(unsigned int) address);

#if DEBUG_LEVEL >= 2
		SETUP_BANK(BANK_RR);
		READ_REG16(OFFSET_RR, data);

		/*
		 * Print out the Chip ID and Revision
		 */
		itc_printf ("SMSC Chip ID: 0x%x\r\n", ((data >> 4) & 0xF));
		itc_printf ("SMSC Chip Revision: 0x%x\r\n", (data & 0xF));
#endif
		return 1;
	}
	
	return 0;
} // End of probe_smsc91c111

/*
 * NOTES:
 * 	When you write data to the SMSC the first 16 bits must be status
 * 	bytes (0's are good) and the second 16 bits is suppose to be the
 * 	size of the ethernet frame.
 *
 * 	The size of the ethernet frame should be the size of the data + 6
 * 	because the size should include the 4 bytes (status and size) and
 * 	2 bytes for the trailer (described later).  Also, the SMSC always
 * 	ignores ODD counts (you send it 11 bytes it reads it as 10 bytes.)
 *
 * 	The trailer is how the SMSC figures out if you are sending an ODD
 * 	number of bytes or not.  The trailer is the last 2 bytes of data.
 * 	If the second byte in the trailer has bit 5 set then the first
 * 	byte in the trailer in a valid byte and should be included in the
 * 	transmission, but if bit 5 in the trailer is 0 then the first byte
 * 	in the trailer is just a pad and is ignored.
 *
 * 	Also note that the auto-increment only works in 32-bit mode, so
 * 	all data is re-aligned on the fly if its not 32-bit aligned already.
 */
static int write_mmu_data (unsigned char packet_number, unsigned char *data, 
		unsigned int data_size)
{
	unsigned short pointer_offset;
	unsigned int *data32=NULL, _temp32;
	unsigned short *data16=NULL;
	unsigned short _temp;
	int counter,i;
	unsigned int alignment, leftovers;

	if (data_size > ((1 << 11) - 1))
		return 0;

	/*
	 * We set up the pointer register to auto-increment.  This is the
	 * fastest way to do it.
	 */	
	SETUP_BANK(BANK_PR);
	counter=MAX_PR_COUNTER;
	do 
	{
		READ_REG16(OFFSET_PR, _temp);
	} while ((_temp & BIT_PR_NOTEMPTY) && --counter);
        if (!counter)
        {
                ERROR("TX FIFO not empty\r\n");
        }

	WRITE_REG16(OFFSET_PR, BIT_PR_AUTO_INC);
	SETUP_BANK(BANK_DR);
	REG32(OFFSET_DR) = ((data_size) + 6) << 16;

	/*
	 * Leftovers has 2 purposes, to figure out our alignment, and send the
	 * remainder of the data at the end.
	 */
	leftovers = ((unsigned int)data % 4);

	if (leftovers == 0)
	{
		alignment = 4;
		data32 = (unsigned int *)data;
	} else if (leftovers == 2)
	{
		alignment = 2;
		data16 = (unsigned short *)data;
	} else
	{
		alignment = 1;
	}

	/*
	 * This was changed when it became apparent that auto-increment may
	 * not have been working right in the 16 and 8 bit write cases.  If
	 * you know it works, this can be redone so that the leftovers
	 * gets data_size % alignment and changes are done in the respective
	 * if statements below (so one REG8 or REG16 is done).
	 *
	 * Order was right, but I saw the MMU increment 32 bits after a 16 bit
	 * access.  (FFEEDDCC) Would become (FFEE0000 DDCC0000).
	 */
	leftovers = (data_size % 4);

	for (pointer_offset=0;pointer_offset < (data_size - leftovers);
		pointer_offset += 4)
	{
		if (alignment == 4)
		{
			REG32(OFFSET_DR) = data32[pointer_offset/4];
		} else if (alignment == 2)
		{
			_temp32 = data16[pointer_offset/2];
			_temp32 |= data16[pointer_offset/2 + 1] << 16;
			REG32(OFFSET_DR) = _temp32;
		} else
		{
			_temp32 = data[pointer_offset];
			_temp32 |= data[pointer_offset + 1] << 8;
			_temp32 |= data[pointer_offset + 2] << 16;
			_temp32 |= data[pointer_offset + 3] << 24;
			REG32(OFFSET_DR) = _temp32;
		}
	}

	/*
	 * Here we send out the remaining bytes and setup the ODD Byte if
	 * we have to.
	 */
	if (leftovers)
	{
		SETUP_BANK(BANK_PR);
                counter=MAX_PR_COUNTER;
		do 
		{
			READ_REG16(OFFSET_PR, _temp);
		} while ((_temp & BIT_PR_NOTEMPTY) && --counter);
                if (!counter)
                {
                        ERROR("TX FIFO not empty, but was\r\n");
                }

		/*
		 * This is necessary as the pointer offset for the DR
		 * is different than our data pointer.  Don't forget that
		 * the first 32-bit word in the DR is the size and status
		 */
		WRITE_REG16(OFFSET_PR, pointer_offset + 4);
	
		_temp32 = 0;
		SETUP_BANK(BANK_DR);
		for (i=0;i<leftovers;i++)
		{
			_temp32 |= data[pointer_offset + i] << (i*8);
		}

		if ((data_size & 1) == 1)
		{
			/*
			 * We have an odd number of bytes
			 */
			_temp32 |= 0x20 << (i*8);
			
		}
		REG32(OFFSET_DR) = _temp32;
	} 

	return 1;
} // End of write_mmu_data

/*
 * Apparently, the SMSC has difficulty with the ODD Byte on receive if you
 * get one that is Rev 0.  Although, there is not documentation on this on
 * the SMSC Website (Errata could not be found) the Linux Driver does this.
 *
 * Also, it only hurts if the upper-levels are unable to figure out that
 * the size is one larger, so the Revision Register (Chip ID and Rev) are
 * checked and if the appropriate Rev and Chip ID is found, the ODD Byte
 * is read regardless of the status.
 *
 * Chip ID == 9 and Rev 0 apparently is the problem chip.
 *
 * Additionally, the auto-increment does not work for anything other than
 * 32-bit accesses (see write_mmu_data comment for more info)
 */
static int read_mmu_data (unsigned char *packet_number, unsigned char *data, 
		unsigned short *data_size)
{
	unsigned short pointer_offset;
	unsigned short size;
	unsigned int *data32, _temp32, data_count;
	int counter;
	unsigned short *data16;
	unsigned short _temp;
	int leftovers,i;

	if (*data_size > ((1 << 11) -1))
	{
		ERROR("%s: Data buffer > max allowable rx packet\r\n",
		      __FUNCTION__);
	}

	/*
	 * Setup the pointer register to auto-increment our addresses so
	 * that we don't have to worry about it.  This should not be a big
	 * deal on hardware that works
	 */	
	SETUP_BANK(BANK_PR);
	counter=MAX_PR_COUNTER;
	do
	{
		READ_REG16(OFFSET_PR, _temp);
	} while ((_temp & BIT_PR_NOTEMPTY) && --counter);

	WRITE_REG16(OFFSET_PR, BIT_PR_RCV | BIT_PR_READ | BIT_PR_AUTO_INC);

	SETUP_BANK(BANK_DR);
	_temp32 = REG32(OFFSET_DR);
	//stat = _temp32 & 0xFFFF;
	size = ((_temp32 >> 16) & 0x7FF);
	// Size includes the status, byte count and control words
	// Ignore the ODD bit for now and read the extra odd byte regardless.
	data_count = size - 6 + 1;

	// Don't overwrite the buffer
	// Don't bother writing the message for an off-by-one truncation
	// because it's probably due to us ignoring the odd byte bit.
	if (data_count-1 > *data_size)
	{	
		DEBUG_2("%s: Data truncated: %d>%d\r\n", __FUNCTION__,
			data_count, *data_size);
		data_count = *data_size;

		// Throw out truncated packet
	        return 0;
	}
        *data_size = data_count;

	/*
	 * Handle the buffer alignment cases (32,16,8)
	 * Try to read the largest words as we can
	 * in as tight a loop as possible.
	 */

        // Read the last 0..3 bytes of data one byte at a time
        leftovers = data_count % 4;
        data_count -= leftovers;

	if (((unsigned int)data % 4) == 0)
	{
		// Buffer area is long word aligned
		data32 = (unsigned int *)data;

		// Convert count into long words
		data_count /= 4;

	        // Read as many whole words as we can
	        for(    pointer_offset=0;
	        	pointer_offset < data_count;
	        	++pointer_offset)
	        {
			data32[pointer_offset] = REG32(OFFSET_DR);
		}

		// Convert pointer_offset into byte offset
		pointer_offset *= 4;
	}
	else if (((unsigned int)data % 4) == 2)
	{
		// Buffer area is short word aligned
		data16 = (unsigned short *)data;

		// Convert count into short words
		data_count /= 2;

	        // Read as many short words as we can
	        for(    pointer_offset=0;
	        	pointer_offset < data_count;
	        	pointer_offset += 2)
	        {
			_temp32 = REG32(OFFSET_DR);
			data16[pointer_offset] = (_temp32) & 0xFFFF;
			data16[pointer_offset +1] = (_temp32 >> 16) & 0xFFFF;
		}

		// Convert pointer_offset into byte offset
		pointer_offset *= 2;
	}
	else
	{
		// Buffer is byte aligned

	        // Read as many short words as we can
	        for(    pointer_offset=0;
	        	pointer_offset < data_count;
	        	pointer_offset += 4)
	        {
			_temp32 = REG32(OFFSET_DR);
			data[pointer_offset] = _temp32 & 0xFF;
			data[pointer_offset + 1] = (_temp32 >> 8) & 0xFF;
			data[pointer_offset + 2] = (_temp32 >> 16) & 0xFF;
			data[pointer_offset + 3] = (_temp32 >> 24) & 0xFF;
		}
	}

        // Read the remaining bytes and store as many as requested
        if (leftovers)
        {
            _temp32 = REG32(OFFSET_DR);
            for (i=0;leftovers--;i++)
            {
                    data[pointer_offset + i] = (_temp32 >> (i*8)) & 0xFF;
            }
        }
	
	return 1;

} // End of read_mmu_data

static int release_rx_packet (void)
{
	unsigned short _temp;
	int counter;

	counter=MAX_MMUCR_COUNTER;
	SETUP_BANK(BANK_MMUCR);
	do 
	{	
		READ_REG16(OFFSET_MMUCR, _temp);
	} while ((_temp & BIT_MMUCR_BUSY) && --counter);
	if (!counter)
	{
		ERROR ("MMU busy\r\n");
	}
	

	WRITE_REG16(OFFSET_MMUCR, OPCODE_MMUCR_REM_REL);

	do 
	{
		READ_REG16(OFFSET_MMUCR, _temp);
	} while (_temp & BIT_MMUCR_BUSY);

	return 1;
} // End of release_rx_packet

/*
 * We must release the tx packet passed to us from the MMU.  If we do not
 * do this, then the memory will fill up.
 */
static int release_tx_packet (unsigned char packet_number)
{
	unsigned short _temp;

	SETUP_BANK (BANK_PNR);
	WRITE_REG8(OFFSET_PNR,packet_number);

        SETUP_BANK (BANK_MMUCR);
        do
        {
                READ_REG16 (OFFSET_MMUCR, _temp);
        } while (_temp & BIT_MMUCR_BUSY);
        
        WRITE_REG16 (OFFSET_MMUCR, OPCODE_MMUCR_REL_SPEC);
	
	return 1;
		

} // End of release_tx_packet

static int allocate_tx (unsigned char *packet_number)
{
	unsigned int counter;
	unsigned short _temp;
	unsigned char _temp8;
	int retry=0;    // disable retries for now

	SETUP_BANK(BANK_MIR);

	/*
	 * We will wait till there is some available memory before
	 * asking for some.
	 */
	counter = MAX_TRY;
	do
	{
		READ_REG16(OFFSET_MIR, _temp);
	} while (((_temp >> 8) == 0) && --counter);

	/* 
	 * For the Case of entering the if condition below...
	 * 
	 * Ok, we can't use the SMSC cause all of its memory is used up.
	 * This can be one of 2 cases.
	 *
	 * 1.  We have Received Frames there.  This does us no good as
	 * we are single threaded.
	 *
	 * 2.  We have Transmitted Frames there.
	 *
	 * 3. Both 1 & 2
	 *
	 * Number 2 is bad, because we should only really ever have one tx
	 * packet in the buffer (we wait for its compeletion).  It should not
	 * happen.
	 *
	 * Number 1 can be fixed by removing packets by either reading them
	 * and releasing them, releasing them individually, or flushing them
	 * all.
	 */
	if (!counter)
	{
		DEBUG_2("%s: Out of MMU Memory; flushing buffers\r\n", __FUNCTION__);
                flush_ethernet ();
		//return 0;
	}

allocate:
	/*
	 * Wait for MMU to become available
	 */
	SETUP_BANK(BANK_MMUCR);
        counter = MAX_TRY;
	do
	{
		READ_REG16(OFFSET_MMUCR, _temp);
	} while (_temp & BIT_MMUCR_BUSY && --counter);
	if (!counter)
	{
		ERROR ("MMU busy\r\n");
	}

	/*
	 * Give the ALLOCATE TX PACKET command to the MMU
	 */
	WRITE_REG16(OFFSET_MMUCR, OPCODE_MMUCR_ALLOC_TX);

	SETUP_BANK(BANK_ISR);
	counter=MAX_TRY;
	do
	{
		READ_REG8(OFFSET_ISR, _temp8);
	} while	(!(_temp8 & BIT_ISR_ALLOC_INT) && --counter);

	if (!counter)
	{
		DEBUG_2("%s: Allocation Timeout\r\n", __FUNCTION__);
		if (!retry++)
		{
			goto allocate;
		}
		return 0;
	}

	/*
	 * If the below fails lets try to flush the ethernet in one last
	 * try to make a go of this.
	 */
	SETUP_BANK(BANK_ARR);
	READ_REG8(OFFSET_ARR, _temp8);
	if (_temp8 & BIT_ARR_FAILED)
	{

		if (!retry++)
		{
		        // We probably got flooded with received packets
		        // at this point, using up all the buffers.
                        DEBUG_2("%s: Allocation failed; flushing buffers\r\n",
                               __FUNCTION__);
			flush_ethernet ();
			goto allocate;
		}
		return 0;
	}
	
	*packet_number &= _temp8 & MASK_ARR_PACKET_NUM;
	
	return 1;
} // End of allocate_tx

/*
 * This function will do the bit-banging neccessary to do the MII transfers
 * to the PHY itself.  Below is a description of how it works.
 *
 * The sequence for the Management Interface is 
 * IDLE->START->OPCODE->PHY_ADDR->REG_ADDR->TA->DATA
 *
 * IDLE is 32 1's over the MDIO (via MI_IDLE)
 * START is 01b over the MDIO (via MI_START) 
 * OPCODE is 01b or 10b over the MDIO (via MI_WRITE / MI_READ)
 * PHY_ADDR is always 00000b (Internal)
 * REG_ADDR is reg (5 lower bits).
 * TA is Turnaround time (don't care for write)
 * DATA is data.
 *
 * reg and data both should be clocked out MSB first (Ie. Bit 15 / 4)
 */
static int write_phy_register (unsigned char reg, unsigned short data)
{
	int i;

	SETUP_BANK(BANK_MI);
	
	/* 
	 * Lets do the IDLE
	 */
	mi_idle();
		
	/*
	 * Lets do that START condition
	 */
	mi_start ();

	/*
	 * Lets tell it we are doing a write
	 */
	mi_write_op ();

	/*
	 * Lets tell it the PHY ADDRESS
	 */
	mi_phy_addr (PHY_INT_ADDR);

	/*
	 * Lets tell it the PHY REG
	 */
	mi_phy_addr (reg);

	/*
	 * Here's The TurnAround time.  This is here so that if the
	 * PHY has to switch from Tx or Rx it has time to do so..
	 */
	mi_ta ();
	
	for (i=15;i>=0;i--)
	{
		WRITE_REG16(OFFSET_MI,((data >> i) & 0x1) | MI_MDOE);
		udelay (MI_CLK_DELAY);
		WRITE_REG16(OFFSET_MI,((data >> i) & 0x1) | MI_MDOE | MI_MDCLK);
		udelay (MI_CLK_DELAY);
	}
		
	return 1;
} // End of write_phy_register


/*
 * This does the bit-banging for the MII to the PHY
 *
 * The sequence for the Management Interface is 
 * IDLE->START->OPCODE->PHY_ADDR->REG_ADDR->TA->DATA
 *
 * IDLE is 32 1's over the MDIO (via MI_IDLE)
 * START is 01b over the MDIO (via MI_START) 
 * OPCODE is 01b or 10b over the MDIO (via MI_WRITE / MI_READ)
 * PHY_ADDR is always 00000b (Internal)
 * REG_ADDR is reg (5 lower bits).
 * TA is Turnaround time (don't care for write)
 * DATA is data.
 *
 * reg and data both should be clocked out MSB first (Ie. Bit 15 / 4)
 */
static int read_phy_register (unsigned char reg, unsigned short *data)
{
	int i;
	unsigned short _temp;

	SETUP_BANK(BANK_MI);
	
	/* 
	 * Lets do the IDLE
	 */
	mi_idle ();
		
	/*
	 * Lets do that START condition
	 */
	mi_start ();

	/*
	 * Lets tell it we are doing a write
	 */
	mi_read_op ();

	/*
	 * Lets tell it the PHY ADDRESS
	 */
	mi_phy_addr (PHY_INT_ADDR);

	/*
	 * Lets tell it the PHY REG
	 */
	mi_phy_addr(reg);

	/*
	 * Heres The TurnAround time.  This is here so that if the
	 * PHY has to switch from Tx or Rx it has time to do so..
	 */
	mi_ta ();

	/*
	 * Read the Data in from the PHY
	 */	
	for (i=15,*data=0;i>=0;i--)
	{
		WRITE_REG16(OFFSET_MI, 0);
		udelay (MI_CLK_DELAY);
		READ_REG16(OFFSET_MIR, _temp);
		*data |= ((((_temp >> 1) & 0x1) << i));
		WRITE_REG16(OFFSET_MI, MI_MDCLK);
		udelay (MI_CLK_DELAY);
	}
		
	return 1;
} // End of read_phy_register

/*
 * This function may be used to print out the Ethernet Packets.  It does
 * the appropriate byte swapping to make it easier for reading (by humans).
 * 
static int print_data (unsigned char *data, unsigned short size)
{

	int i=0,x=0,leftovers;
	unsigned int data32=0;

	DEBUG("print_data: Printing %x bytes\r\n", size);

	for (i=0;i<(size - 3);i+=4)
	{
		if ((i % 24)==0)
		{
			itc_printf("\r\n\t");
		}
		
		data32 = data[i]	<< 24;
		data32 |= data[i+1] 	<< 16;
		data32 |= data[i+2] 	<< 8;
		data32 |= data[i+3];
		itc_printf("%x ", data32);

	}

	if (i < (size))
	{
		leftovers = size - i;
		for (data32=0,x=leftovers - 1;x >=0;i++,x--)
		{
			data32 |= data[i] << (8*x);
		}
		itc_printf("%x (%i bytes valid)", data32, leftovers);
	}

	itc_printf("\r\n");
	return 1;
}
*/

int rx_ethernet_off (void)
{
	/*
	 * Disable Receive
	 */
	SETUP_BANK(BANK_RCR);
	REG16(OFFSET_RCR) &= ~(BIT_RCR_RXEN);

	return 1;
}

int flush_ethernet (void)
{
	unsigned short _temp;
	int counter;

	/*
	 * Reset MMU
	 *
	 * This will flush all of Memory (both tx and rx).
	 */
	SETUP_BANK (BANK_MMUCR);
        counter = MAX_TRY;
	do 
	{
		READ_REG16(OFFSET_MMUCR, _temp);
	} while (_temp & BIT_MMUCR_BUSY && --counter);
	if (!counter)
	{
		ERROR ("MMU busy\r\n");
	}
	
	WRITE_REG16(OFFSET_MMUCR, OPCODE_MMUCR_RESET);

	do 
	{
		READ_REG16(OFFSET_MMUCR, _temp);
	} while (_temp & BIT_MMUCR_BUSY && --counter);

	//SETUP_BANK(OFFSET_MIR);
	return 1;
}

int rx_ethernet_on (void)
{
	SETUP_BANK(BANK_RCR);
	REG16(OFFSET_RCR) |= BIT_RCR_RXEN;
	return 1;
}
