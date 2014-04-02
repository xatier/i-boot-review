//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      serial.h
//
// Description:
//
//      Interfaces with the StrongArm SA1110 serial hardware.
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

#ifndef SERIAL_H
#define SERIAL_H

#ifndef ASM
#include <types.h>
#endif

#ifdef XSCALE
#define SERIAL_BAUD_9600   (0x00000060)
#define SERIAL_BAUD_19200  (0x00000030)
#define SERIAL_BAUD_38400  (0x00000018)
#define SERIAL_BAUD_57600  (0x00000010)
#define SERIAL_BAUD_115200 (0x00000008)
#else
#define SERIAL_PORT 2
#define SERIAL_ECHO

#define SERIAL_BAUD_9600   (0x00000017)
#define SERIAL_BAUD_19200  (0x0000000B)
#define SERIAL_BAUD_38400  (0x00000005)
#define SERIAL_BAUD_57600  (0x00000003)
#define SERIAL_BAUD_115200 (0x00000001)

//UART Control Registers
#define UTCR0(x) (0x80010000 + (x * 0x00020000))
#define UTCR1(x) (0x80010004 + (x * 0x00020000))
#define UTCR2(x) (0x80010008 + (x * 0x00020000))
#define UTCR3(x) (0x8001000C + (x * 0x00020000))

//UART Status Registers
#define UTSR0(x) (0x8001001C + (x * 0x00020000))
#define UTSR1(x) (0x80010020 + (x * 0x00020000))

#define UTDR(x) (0x80010014 + (x * 0x00020000))

#define SERIAL_UTCR0 (*(volatile u32 *)(UTCR0(SERIAL_PORT)))
#define SERIAL_UTCR1 (*(volatile u32 *)(UTCR1(SERIAL_PORT)))
#define SERIAL_UTCR2 (*(volatile u32 *)(UTCR2(SERIAL_PORT)))
#define SERIAL_UTCR3 (*(volatile u32 *)(UTCR3(SERIAL_PORT)))

#define SERIAL_UTSR0 (*(volatile u32 *)(UTSR0(SERIAL_PORT)))
#define SERIAL_UTSR1 (*(volatile u32 *)(UTSR1(SERIAL_PORT)))

#define SERIAL_UTDR (*(volatile u32 *)(UTDR(SERIAL_PORT)))

#define SERIAL_UTCR0_ASM UTCR0(SERIAL_PORT)
#define SERIAL_UTCR1_ASM UTCR1(SERIAL_PORT)
#define SERIAL_UTCR2_ASM UTCR2(SERIAL_PORT)
#define SERIAL_UTCR3_ASM UTCR3(SERIAL_PORT)

#define SERIAL_UTSR0_ASM UTSR0(SERIAL_PORT)
#define SERIAL_UTSR1_ASM UTSR1(SERIAL_PORT)

#define UTSR1_TX_BSY (0x00000001)
#define UTSR1_RX_NOT_EMPTY (0x00000002)
#define UTSR1_RX_PARITY_ERROR (0x00000008)
#define UTSR1_RX_FRAMING_ERROR (0x00000010)
#define UTSR1_RX_OVERFLOW (0x00000020)

#define UTSR0_TX_HALF_EMPTY (0x00000001)

#define UTCR0_1_SBIT (0x00000000)
#define UTCR0_8_DBIT (0x00000008)

#define UTCR3_RX_ON (0x00000001)
#define UTCR3_TX_ON (0x00000002)
#endif

#ifndef ASM
void init_serial(u32 baud);
void output_byte_serial(char byte);
void output_string_serial(char const *string);
int input_byte_serial(char *byte);
int get_line_serial(char *buf, int buflen);
int input_line_serial(char *buf, int buflen);
int raw_input_serial(char *byte, u32 uCount, int timeout);
#endif

#endif //SERIAL_H
