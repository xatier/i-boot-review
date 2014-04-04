////////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      c_main.c
//
// Description:
//
//      Initializes anything that needs to be initialized, and starts the UI or
//      default OS.
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

#include <c_main.h>
#include <serial.h>
#include <flash_i128.h>
#include <timer.h>
#include <os.h>
#include <ui.h>
#include <parser.h>
#include <arp.h>
#include <ethernet.h>
#include <string.h>
#include <util.h>
#include <checksum.h>

#if XSCALE
#include <start_xscale.h>
#include <i2c_boot.h>
#include <eeprom_24lc64.h>
#else
#include <start_sa.h>
#endif

#ifdef TAGGED_EEPROM
#include <eeprom.h>
#endif

#include <pcmcia.h>

iboot_status status;

const char copyright[] = "Copyright 2001,2002 Intrinsyc Software Inc.";

extern int bootmem_parse(char const *arg);
//void inc_led(void);
int c_main(void);

////////////////////////////////////////////////////////////////////////////////
// init_status
// PURPOSE: Initializes status.
// PARAMS:  None.
// RETURNS: Nothing.
////////////////////////////////////////////////////////////////////////////////
static void
init_status(void)
{
    volatile unsigned char *ram_bit;

    status.dl = dl_tftp;
    status.os = Linux;
    status.CEKernel = 0x0;
    status.ciaddr = 0x0;
    status.siaddr = 0x0;
    status.giaddr = 0x0;
    status.smask = 0x0;
    status.macaddr[0] = 0x0300;
    status.macaddr[1] = 0x002A;
    status.macaddr[2] = 0x4A22;
#ifdef XSCALE
    status.processor = xscale;
#else
    status.processor = sa1110;
#endif
#ifdef INTERLEAVED
    status.flash_chip = i128x2;
#else
    status.flash_chip = i128;
#endif

	status.memsize = RAM_SIZE*1024*1024;
}

//Just testing...
/*
void init_wakeup()
{
   if(*((volatile u32*)PSSR) & 0x00000001)
   {
      void (*kernel)() = (void (*)())((u32 *)(0xc0058000));

      kernel();
   }
}
*/

////////////////////////////////////////////////////////////////////////////////
// c_main
// PURPOSE: This is the function that is jumped to after IBoot is loaded into
//          memory. Everything is initialized from here.
// PARAMS:  None.
// RETURNS: Does not return.
////////////////////////////////////////////////////////////////////////////////
int
c_main(void)
{
   const char spaces[] = "              ";      // filler spaces

//#ifdef XSCALE
//	*(volatile unsigned int *)(GPIO_BASE + GPSR0) = 0x10;
//#endif

   //init all the devices we need.
   init_status();

   // xatier: in src/main/start_sa.S
   // initialize the serial port
   init_serial(SERIAL_BAUD_115200);

   // xatier: print the messages on the screen
#define LINE_WIDTH (50-5)
   itc_printf("\r\n"
	      "**************************************************\r\n"
	      "** Intrinsyc Bootloader (IBoot)                 **\r\n"
	      "** %s%s**\r\n"
	      "** Version: %s                    %s**\r\n"
	      "** Support: http://www.intrinsyc.com            **\r\n"
	      "**************************************************\r\n",
	      copyright, &spaces[sizeof(spaces)-LINE_WIDTH+sizeof(copyright)-2],
	      VERSION, &spaces[sizeof(spaces)-LINE_WIDTH+sizeof(VERSION)+29-2]
	     );

   // xatier: in src/libs/base/flash_i128.c
   // actually do nothing
   init_flash();

   // xatier: in src/libs/net/checksum.c
   // actually do noting, use a static table crc32_tab[] instead
   init_crc_table();

   // xatier: in src/libs/base/timer_sa.c
   // set RCNR to zero
   init_timer();
   //inc_led();

#if defined(CS8900A_ETHERNET) && defined(XSCALE)
   //Crystal chip can be buggy in the first second or so after booting.
   //Doesn't matter on SA1110, because it will take at least 2 seconds for the
   //RTC to stabilize anyway.
   delay(1);
#endif

//#ifdef XSCALE
//   if (!init_i2c ())
//   {
//      itc_printf("ERROR: There is a problem initializing I2C Controller\r\n");
//   }
//#endif

//#ifdef SMSC91C111_ETHERNET
//   if (!init_eeprom ())
//   {
//      itc_printf("ERROR: There is a problem initializing the EEPROM\r\n");
//   }
//#endif

//#ifdef TAGGED_EEPROM
//   if (!init_eeprom_tagged())
//   {
//      itc_printf("ERROR: There is a problem with the contents of the EEPROM\r\n");
//   }
//#endif

   // xatier: in src/libs/base/ethernet_cs8900a.c
   // initializes the cs8900a ethernet chip
   init_ethernet(status.macaddr);

   // Seed the random number generator with as much entropy as we have

   // xatier: in src/libs/base/timer_sa.c
   // returns RCNR
   srand(get_time_timer() ^ ((status.macaddr[2] << 16) | status.macaddr[1]));

   //Bring up the user interface. This will return on a timeout or through user
   //direction. This will not return if the user tells us to boot.
   // xatier: in src/libs/tools/ui.c
   // set the serial port ui with a timeout
   init_ui(UI_TIMEOUT, mode_default);

   //inc_led();

   // xatier: in src/libs/base/pcmcia.c
   // an I/O routines/driver with pcmica interface
   // pcmica(i)
   //
   //   i -> 1 insert a pcmcia card, try to find the `kernel` and `ramdisk.gz`
   //          assume the file system is formatted in FAT32
   //
   //   i -> 0 eject a pcmcia card
   //
   //   data structures are defined in src/include/pcmcia.h
   //   in src/libs/base/pcmcia.c, also defined some functions for IDE and VFAT
   //
   itc_strcpy(kname, "kernel");
   if(pcmcia(1)==0) {
      // xatier: in src/libs/tools/parser.c
      // parse the kernel type and bootup address
      // invokes boot_linux_kernel() in src/libs/base/os.c
      // the kernel starting physical address is 0xa0008000
      // start the linux kernel here
      bootmem_parse("linux 0xa0008000");
   }

   // xatier: in src/libs/base/os.c
   // bring the operation system up, acctually a wrapper of another function run_os()
   // brings the default os up to a bootable state, then boots it
   //Load/decode an os, and start it. This should never return.
   init_os();

   // xatier: fall back operations
   //If init_os does return, drop the user to the prompt.
   init_ui(0, mode_default);

   return 0;
}
