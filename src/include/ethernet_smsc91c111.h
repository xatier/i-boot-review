//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      ethernet_smsc91c111.h
//
// Description:
//
//      Driver for the SMCS 91C111 Ethernet chip.
//
// Author:
//
//      Brad Remedios
//
// Created:
//
//      February 2002
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETHERNET_SMSC91C111_H__
#define __ETHERNET_SMSC91C111_H__

#include <timer.h>

/*
 * Defines as per where to find the SMSC LAN91C111
 */
#ifdef SECOND_ETHERNET
#define SMSC91C111_WINDOW_ADDR	0x08000000	// (Static Chip Select 2)
#else
#define SMSC91C111_WINDOW_ADDR	0x0C000000	//0x04000000
#endif
#define EEPROM_MAC_ADDR		0x0010
#define IO_BASE			0x300
#define PHY_INT_ADDR		0x00

/*
 * This is the maximum number of tries before bailing on a busy wait
 */
#define MAX_RX_RETRY		30
#define MAX_TRY			4000
#define MAX_RETRANSMIT		10
#define MAX_ISR_COUNTER		6000
#define PHY_RESET_WAIT		3000
#define MAX_PR_COUNTER		4096
#define MAX_MMUCR_COUNTER	3000

/*
 * Values for determining if we are SMSC LAN91C111
 */
#define BSR_UNIQUE		0x3300

/*
 * Register Offset Definitions.  Some of these are the same, because they
 * exist on a different bank.
 */
#define OFFSET_BSR		0xE	// Bank Select Register
#define OFFSET_TCR		0x0	// Transmit Control Register
#define OFFSET_EPHSR		0x2	// EPH Status Register
#define OFFSET_RCR		0x4	// Receive Control Register
#define OFFSET_CNTREG		0x6	// Counter Register
#define OFFSET_MIR		0x8	// Memory Information Register
#define OFFSET_RPCR		0xA	// Receive/Phy Control Register

#define OFFSET_CNFGR		0x0	// Configuration Register
#define OFFSET_BAR		0x2	// Base Address Register
#define OFFSET_IAR		0x4	// Individual Address Register (4-9)
#define OFFSET_IAR01		0x4	// Individual Address 0 & 1
#define OFFSET_IAR23		0x6	// Individual Address 2 & 3
#define OFFSET_IAR45		0x8	// Individual Address 4 & 5
#define OFFSET_GPR		0xA	// General Purpose Register
#define OFFSET_CR		0xC	// Control Register

#define OFFSET_MMUCR		0x0	// MMU Command Register
#define OFFSET_PNR		0x2	// Packet Number Register
#define OFFSET_ARR		0x3	// Allocation Result Register
#define OFFSET_FPR		0x4	// Fifo Ports Register
#define OFFSET_PR		0x6	// Pointer Register
#define OFFSET_DR		0x8	// Data Register
#define OFFSET_ISR		0xC	// Interrupt Status Register
#define OFFSET_IRQAR		0xC	// Interrupt Acknowledge Register
#define OFFSET_IMR		0xD	// Interrupt Mask Register

#define OFFSET_MI		0x8	// Management Interface
#define OFFSET_RR		0xA	// Revision Register

/*
 * Register Bank Definitions.
 */
#define BANK_BSR		0x0	// Bank Select Register (All Banks)
#define BANK_TCR		0x0	// Transmit Control Register
#define BANK_EPHSR		0x0	// EPH Status Register
#define BANK_RCR		0x0	// Receive Control Register
#define BANK_CNTREG		0x0	// Counter Register
#define BANK_MIR		0x0	// Memory Information Register
#define BANK_RPCR		0x0	// Receive/Phy Control Register

#define BANK_CNFGR		0x1	// Configuration Register
#define BANK_BAR		0x1	// Base Address Register
#define BANK_IAR		0x1	// Individual Address Register
#define BANK_GPR		0x1	// General Purpose Register
#define BANK_CR			0x1	// Control Register

#define BANK_MMUCR		0x2	// MMU Command Register
#define BANK_PNR		0x2	// Packet Number Register
#define BANK_ARR		0x2	// Allocation Result Register
#define BANK_FPR		0x2	// Fifo Ports Register
#define BANK_PR			0x2	// Pointer Register
#define BANK_DR			0x2	// Data Register
#define BANK_ISR		0x2	// Interrupt Status Register
#define BANK_IRQAR		0x2	// Interrupt Acknowledge Register
#define BANK_IMR		0x2	// Interrupt Mask Register

#define BANK_MI			0x3	// Management Interface
#define BANK_RR			0x3	// Revision Register

/*
 * Bit Definitions
 */
#define BIT_TCR_SWFDUP		0x8000	// TCR Switched Full Duplex
#define BIT_TCR_EPHLOOP		0x2000	// EPH Block Internal Loop
#define BIT_TCR_STPSQET 	0x1000	// Stop TX on SQET
#define BIT_TCR_FDUPLX		0x0800	// Full-Duplex
#define BIT_TCR_MON_CSN		0x0400	// Monitor Carrier
#define BIT_TCR_NOCRC		0x0100	// No CRC Appended
#define BIT_TCR_PAD_EN		0x0080	// Pad Enable (Tx Frames < 64 bytes)
#define BIT_TCR_FORCOL		0x0004	// Force Collision
#define BIT_TCR_LOOP		0x0002	// Loopback
#define BIT_TCR_TXENA		0x0001	// Transmit Enable

#define BIT_EPHSR_TXUNRUN	0x8000	// TX UnderRun
#define BIT_EPHSR_LINK_OK	0x4000	// Link Ok
#define BIT_EPHSR_TX_DEFR	0x0080	// Tx deferred
#define BIT_EPHSR_TX_SUC	0x0001	// Tx Success

#define BIT_RCR_SOFTRST		0x8000	// Soft Reset
#define BIT_RCR_ABORT_ENB	0x2000	// Collision Abort Enable
#define BIT_RCR_STRIP_CRC	0x0200	// Strip CRC
#define BIT_RCR_RXEN		0x0100	// RX Enable
#define BIT_RCR_PRMS		0x0002	// Promiscuous Mode

#define BIT_RPCR_SPEED		0x2000	// Speed Select (10/100)
#define BIT_RPCR_DPLX		0x1000	// Duplex Select (Full/Half)
#define BIT_RPCR_ANEG		0x0800	// Auto Negotiation

#define RPCR_LINK_LED		0x0	// 10 / 100 Link
#define RPCR_100_LED		0x5	// LED 100 MBit Link
#define RPCR_10_LED		0x2	// LED 10 MBit Link
#define RPCR_TX_RX_LED		0x4	// LED Tx/Rx Activity

#define BIT_CR_RCV_BAD		0x4000	// Receive bad CRC Packets
#define BIT_CR_AUTO_RELEASE	0x0800	// Auto Release Successful Tx's
#define BIT_CR_LE_ENABLE	0x0080	// Link Error Enable
#define BIT_CR_CR_ENABLE	0x0040	// Counter Rollover Enable
#define BIT_CR_TE_ENABLE	0x0020	// Transmit Error Enable
#define BIT_CR_EEPROM_SELECT	0x0004	// Select EEPROM / GPR
#define BIT_CR_RELOAD		0x0002	// Reload from EEPROM
#define BIT_CR_STORE		0x0001	// Store to EEPROM
#define BIT_MMUCR_BUSY		0x0001	// MMU Busy

#define BIT_CNFGR_EXT_PHY	0x0200	// External PHY
#define BIT_CNFGR_EPH_PWR_EN	0x8000	// EPH Power Enable (not low power)

#define BIT_ARR_FAILED		0x0080	// Allocation Failed
#define BIT_FPR_TEMPTY		0x0080	// TX Empty
#define BIT_FPR_REMPTY		0x8000	// RX Empty

#define BIT_PR_RCV		0x8000	// Receive
#define BIT_PR_READ		0x2000	// Read Operation`
#define BIT_PR_NOTEMPTY		0x0800	// Not Empty
#define BIT_PR_AUTO_INC		0x4000	// Auto Increment

#define MI_MDOE			0x0008	// MD Output Enable
#define MI_MDCLK			0x0004	// MD Clock
#define MI_MDI			0x0002	// MD Input
#define MI_MDO			0x0001	// MD Output
#define MI_CLK_DELAY		100	// 100 microsecond delay

#define BIT_ISR_RCV_INT		0x01	// Receive Interrupt
#define BIT_ISR_TX_INT		0x02	// Transmit Interrupt
#define BIT_ISR_TX_EMPTY_INT	0x04	// Transmit FIFO Empty Interrupt
#define BIT_ISR_ALLOC_INT	0x08	// Allocation Interrupt
#define OPCODE_MMUCR_RESET	0x0040	// Reset MMU
#define OPCODE_MMUCR_ALLOC_TX	0x0020	// Allocate Memory For Tx
#define OPCODE_MMUCR_REM_REL	0x0080	// Remove and Release RX Packet
#define OPCODE_MMUCR_ENQUEUE_TX	0x00C0	// Enqueue TX Packet
#define OPCODE_MMUCR_REL_SPEC	0x00A0	// Release Specific Packet
#define OPCODE_MMUCR_RESET_TX	0x00E0

/*
 * Masks
 */
#define MASK_OFFSET		0xF
#define MASK_BSR_BS		0x7

#define MASK_RPCR_LSA		0x0E
#define MASK_RPCR_LSB		0x1C

#define MASK_MMUCR_OPCODE	0xE

#define MASK_ARR_PACKET_NUM	0x3F
#define MASK_PNR_PACKET_NUM	0x3F
#define MASK_FPR_PACKET_NUM	0x3F

/*
 * Defines for the PHY
 */
#define PHY_CR			0
#define PHY_CR_RST		0x8000	// PHY Reset
#define PHY_CR_LPBK		0x4000	// PHY Loopback
#define PHY_CR_SPEED		0x2000	// PHY SPEED Select (100/10) 
#define PHY_CR_ANEG_EN		0x1000	// PHY Auto Negotiation
#define PHY_CR_PDN		0x0800	// Power Down
#define PHY_CR_MII_DIS		0x0400	// MII Disable
#define PHY_CR_ANEG_RST		0x0200	// Auto Negotiation Reset
#define PHY_CR_DPLX		0x0100	// Duplex Mode (Full / Half)
#define PHY_CR_COLST		0x0080	// Collision Test

#define PHY_SR			1
#define PHY_SR_CAP_T4		0x8000	// 100Base-T4 Capable
#define PHY_SR_CAP_TXF		0x4000	// 100Base-TX Full Duplex
#define PHY_SR_CAP_TXH		0x2000	// 100Base-TX Half Duplex
#define PHY_SR_CAP_TF		0x1000	// 10Base-T Full Duplex
#define PHY_SR_CAP_TH		0x0800	// 10Base-T Half Duplex
#define PHY_SR_CAP_SUPR		0x0040	// MII Preamble Suppression
#define PHY_SR_ANEG_ACK		0x0020	// Auto Negotiation Acknowledge
#define PHY_SR_REM_FLT		0x0010	// Remote Fault Detect
#define PHY_SR_CAP_ANEG		0x0008	// Able to Auto Negotiate
#define PHY_SR_LINK		0x0004	// Link Status
#define PHY_SR_JAB		0x0002	// Jabber Detect
#define PHY_SR_EXREG		0x0001	// Extended Capability Register

#define PHY_ID0			2
#define PHY_ID0_CMPY_ID		0xFFFF	// Company ID Mask
#define PHY_ID1			3
#define PHY_ID1_CMPY_ID		0xFC00	// Company ID Mask
#define PHY_ID1_MANF_ID		0x03F0	// Manufacturer's ID
#define PHY_ID1_MANF_REV	0x000F	// Manufacturer's Revision

#define PHY_ANAR		4
#define PHY_ANAR_NP		0x8000	// Next Page
#define PHY_ANAR_ACK		0x4000	// Acknowledge
#define PHY_ANAR_RF		0x2000	// Remote Fault
#define PHY_ANAR_T4		0x0200	// 100Base-T4 Capable
#define PHY_ANAR_TX_FDX		0x0100	// 100Base-TX Full Duplex Capable
#define PHY_ANAR_TX_HDX		0x0080	// 100Base-TX Half Duplex Capable
#define PHY_ANAR_10_FDX		0x0040	// 10Base-T Full Duplex Capable
#define PHY_ANAR_10_HDX		0x0020	// 10Base-T Half Duplex Capable
#define PHY_ANAR_CSMA		0x0001	// 802.3 CSMA Capable



/*
 * Here are some helper Macros for the MAC
 */


/*
 * These are macros to help with reading and writing to specific
 * registers
 */
#define __REG32(addr)		(*((volatile unsigned int *)(addr)))
#define __REG16(addr)		(*((volatile unsigned short *)(addr)))
#define __REG8(addr)		(*((volatile unsigned char *)(addr)))

#define REG32(reg)	__REG32(SMSC91C111_WINDOW_ADDR + ((IO_BASE + (reg)) << 2))
#define REG16(reg)	__REG16(SMSC91C111_WINDOW_ADDR + ((IO_BASE + (reg)) << 2))
#define REG8(reg)	__REG8(SMSC91C111_WINDOW_ADDR + ((IO_BASE + (reg)) << 2))

#define READ_REG16(reg, val) (val) = REG16(reg)
#define READ_REG8(reg, val) (val) = REG8(reg)
#define WRITE_REG16(reg, val) REG16(reg) = (val)
#define WRITE_REG8(reg, val) REG8(reg) = (val)

/*
 * This macro will setup the bank that you wish to use.
 */
#define SETUP_BANK(bank)	REG16(OFFSET_BSR) = ((bank) & MASK_BSR_BS)


/*
 * Here are some Macros for the PHY.
 */


/*
 * This is the IDLE State for the Management Interface..
 * Basically, clock out 32 1's.  This is put 
 *
 * var is a temp that will get corrupted
 *
 * So basically MDO will have 32 1's written out over it, and there
 * will be 32 cycles (32 high MDCLKS and 32 low MDCLKS).
 */
static inline void mi_idle (void)
{
	int i;
	
	for(i=0;i<32;i++)
	{
		WRITE_REG16(OFFSET_MI, MI_MDO | MI_MDOE); 
		udelay(MI_CLK_DELAY); 
		WRITE_REG16(OFFSET_MI, MI_MDO | MI_MDOE | MI_MDCLK); 
		udelay(MI_CLK_DELAY); 
	}
}	

/*
 * This is the start state for the Management Interface.
 *
 * Start state = 01b following an IDLE.
 */
static inline void mi_start (void)
{
	WRITE_REG16(OFFSET_MI, MI_MDOE);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO | MI_MDCLK);
	udelay(MI_CLK_DELAY);
}

/*
 * This is the read opcode for the Management Interface.
 *
 * Read Opcode = 10b following start state
 */
static inline void mi_read_op (void) 
{
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO | MI_MDCLK);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
} 

/*
 * This is the write opcode for the Management Interface
 *
 * Write Opcode = 01b following start state
 */
static inline void mi_write_op (void)
{
	WRITE_REG16(OFFSET_MI, MI_MDOE);
       	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDCLK); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO | MI_MDCLK); 
	udelay(MI_CLK_DELAY); 
}

/*
 * This is the part to write out the data for both the PHY Address and
 * REG Address as they are both 5 bits in length.  
 *
 * This get written in both the read and write cycles.
 *
 * I basically unrolled the loop in here because you can make it go faster
 * by doing this, (you gain on the instruction cache side of things.)
 *
 * This is also very convenient because, bit 0 is MDO (Output) so I just
 * shift the data to the right and & out the extra, then add the extra
 * stuff.
 */
static inline void mi_phy_addr (unsigned char addr)
{
	WRITE_REG16(OFFSET_MI, ((addr >> 4) & 0x1) | MI_MDOE); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, ((addr >> 4) & 0x1) | MI_MDOE | MI_MDCLK); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, ((addr >> 3) & 0x1) | MI_MDOE); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, ((addr >> 3) & 0x1) | MI_MDOE | MI_MDCLK); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, ((addr >> 2) & 0x1) | MI_MDOE); 
	udelay(MI_CLK_DELAY); 
	WRITE_REG16(OFFSET_MI, ((addr >> 2) & 0x1) | MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, ((addr >> 1) & 0x1) | MI_MDOE);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, ((addr >> 1) & 0x1) | MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, (addr & 0x1) | MI_MDOE);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, (addr & 0x1) | MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
}

static inline void mi_ta (void)
{
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO); 
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDO | MI_MDCLK);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE);
	udelay(MI_CLK_DELAY);
	WRITE_REG16(OFFSET_MI, MI_MDOE | MI_MDCLK);
	udelay(MI_CLK_DELAY);
}

/*
 * Here Starts Functions that are not to be exported out of
 * ethernet_smsc91c111.c
 */

/*****************************************************************************
 * FUNCTION:
 * 	probe_smsc91c111
 *
 * PURPOSE:
 * 	To determine if an SMSC91c111 exists at the address that is specified
 *
 * IN:
 * 	unsigned int *address - The address to start at.
 *
 * RETURNS:
 * 	1 - SMSC LAN91C111 Found
 * 	0 - No Device Found / Error
 ****************************************************************************/
static int probe_smsc91c111 (unsigned int *address);

/*****************************************************************************
 * FUNCTION:
 * 	write_mmu_data
 *
 * PURPOSE:
 * 	To write data to the MMU's memory.  This function assumes that the
 * 	work to Allocate the Packets Memory has already been done and that we
 * 	were given the right packet number.
 *
 * IN:
 * 	unsigned char packet_number - This is a 6 bit value that specifies
 * 		what the packet number that we are copying for is.
 *
 * 	unsigned char *data - This is the data to write out the ethernet
 * 		device.
 *
 * 	unsigned int data_size - This is the amount of data to write.  
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 *
 * SPECIAL NOTES:
 * 	This is only really used by the tx_packet_ethernet function.
 *
 * 	The largest packet that is able to be transmitted to the SMSC is
 * 	2K (2048 bytes) in size.
 *
 * 	This function accesses or modifies the Packet Number Register,
 * 	Pointer Register and Data Register.
 ****************************************************************************/
static int write_mmu_data (unsigned char packet_number, unsigned char *data,
		unsigned int data_size);

/*****************************************************************************
 * FUNCTION:
 * 	read_mmu_data
 *
 * PURPOSE:
 * 	To read data from the MMU's memory.  This function assumes that there
 * 	is in fact a valid packet waiting in the MMU.
 *
 * IN:
 * 	unsigned int data_size - This is the number of bytes to be read from
 * 		the SMSC's MMU.  
 *
 * OUT:
 * 	unsigned char *packet_number - This is the number of the packet that
 * 		we read.  This packet number is used by the SMSC
 * 		LAN91C111 only, and is a 6 bit value.
 *
 * 	unsigned char *data - This is where the packet data is going.
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int read_mmu_data (unsigned char *packet_number, unsigned char *data,
		 unsigned short *data_size);

/*****************************************************************************
 * FUNCTION:
 * 	allocate_tx
 *
 * PURPOSE:
 * 	To allocate memory in the MMU for a TX Packet.
 *
 * OUT:
 * 	unsigned char *packet_number - This is where we will be storing
 * 		the packet number that we were assigned.
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int allocate_tx (unsigned char *packet_number);

/*****************************************************************************
 * FUNCTION:
 * 	release_rx_packet
 *
 * PURPOSE:
 * 	To release and remove an rx packet from the top of the fifo.
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int release_rx_packet (void);
/*****************************************************************************
 * FUNCTION:
 * 	release_tx_packet
 *
 * PURPOSE:
 * 	To release and remove an tx packet from the top of the fifo.
 *
 * IN:
 * 	unsigned char packet_number - The packet number to release
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int release_tx_packet (unsigned char packet_number);

/*****************************************************************************
 * FUNCTION:
 * 	write_phy_register
 *
 * PURPOSE:
 * 	To write data to the phy registers.
 *
 * IN:
 * 	unsigned char reg - This is the 5 bit offset for which phy register
 * 		should be written to.
 *
 * 	unsigned short data - This is what to set the phy register to.
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int write_phy_register (unsigned char reg, unsigned short data);

/*****************************************************************************
 * FUNCTION:
 * 	read_phy_register
 *
 * PURPOSE:
 * 	To read data from the phy registers
 *
 * IN:
 * 	unsigned char reg - This is the 5 bit offset for which phy register
 * 		would be read from.
 *
 * OUT:
 * 	unsigned short *data - This is where to store the data read from the
 * 		phy
 *
 * RETURNS:
 * 	1 - Success
 * 	0 - Error
 ****************************************************************************/
static int read_phy_register (unsigned char reg, unsigned short *data);

#endif // __ETHERNET_SMSC91C111_H__
