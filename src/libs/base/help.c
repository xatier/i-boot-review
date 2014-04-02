////////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
// 
// Module name:
//
//	help.c
//    
// Description:
//
//	This file contains the online help for I-Boot.
// 
// Author:
//
//	Alfred Pang
// 
// Created: 
//
//	October 2001
// 
////////////////////////////////////////////////////////////////////////////////

#include <help.h>
#include <parser.h>
#include <string.h>

static const THelpText helpText[] = {
{
"boot",
"\
NAME\r\n\
	boot - Boot an image.\r\n\
\r\n\
SYNOPSIS\r\n\
	boot [\"<parameters>\"]\r\n\
\r\n\
DESCRIPTION\r\n\
	Attempts to detect and start a WinCE .bin file, or a Linux kernel\r\n\
	in flash. The kernel must start in flash block 3.\r\n\
\r\n\
ARGUMENTS\r\n\
	parameters - Parameter string to pass to kernel.\r\n\
\r\n\
USAGE\r\n\
	boot\r\n\
"

},

{
"bootme",
"\
NAME\r\n\
	bootme - Announce the device's presence to MS Platform Builder.\r\n\
\r\n\
SYNOPSIS\r\n\
	bootme\r\n\
\r\n\
DESCRIPTION\r\n\
	Broadcasts a MS Platform Builder \"bootme\" message to announce the\r\n\
	device's presence to any instances of such on the subnet.\r\n\
\r\n\
USAGE\r\n\
	bootme\r\n\
"

},

{
"bootmem",
"\
NAME\r\n\
	bootmem - Boot an image at a specified memory address.\r\n\
\r\n\
SYNOPSIS\r\n\
	bootmem <linux | ce> <base address> [\"parameters\"]\r\n\
\r\n\
DESCRIPTION\r\n\
	Starts the Linux or CE kernel located at the base address\r\n\
	with the specified parameters.\r\n\
\r\n\
ARGUMENTS\r\n\
	base address - Memory address at which kernel is located.\r\n\
	parameters - Parameter string to pass to kernel.\r\n\
\r\n\
USAGE\r\n\
	bootmem linux 0x100000\r\n\
"

},

{
"copy",
"\
NAME\r\n\
	copy - Copy data from a RAM source to a RAM destination.\r\n\
\r\n\
SYNOPSIS\r\n\
	copy <destination> <source> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	Copies data of a specified <length> from a <source>\r\n\
	address to a <destination> address. Overlapped <source> and\r\n\
	<destination> memory areas are allowed. Non-aligned addresses are\r\n\
	allowed.\r\n\
\r\n\
ARGUMENTS\r\n\
	destination - Memory address destination.\r\n\
	source - Memory address source.\r\n\
	length - Number of bytes to copy.\r\n\
\r\n\
USAGE\r\n\
	copy 0xc0008000 0xc000A000 0x1000\r\n\
"

},

{
"crc",
"\
NAME\r\n\
	crc - Calculate CRC-32 over memory range.\r\n\
\r\n\
SYNOPSIS\r\n\
	crc <start> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	Calculates the CRC-32 for the memory range from the <start> for\r\n\
	<length> addresses. This CRC can be compared with the expected\r\n\
	CRC to detect corrupted flash images or corrupted downloads.\r\n\
	The CRC is calculated in the same way as in ZIP format archives.\r\n\
\r\n\
ARGUMENTS\r\n\
	start - Base address to calculate.\r\n\
	length - Number of bytes to calculate.\r\n\
\r\n\
USAGE\r\n\
	crc 0xc0000000 807593\r\n\
"
},

{
"createfis",
"\
NAME\r\n\
	createfis - Create a RedBoot-compatible partition table\r\n\
\r\n\
SYNOPSIS\r\n\
	createfis <length1> [<length2> ...]\r\n\
\r\n\
DESCRIPTION\r\n\
	Creates a partition table in the last block of flash in RedBoot format.\r\n\
	The Linux kernel can use this information to set up its MTD partitions.\r\n\
	The size arguments are the sizes of each partition in units of flash\r\n\
	blocks, in sequential order, starting with I-Boot itself in block 0.\r\n\
	createfis should be run before booting the kernel for the first time\r\n\
	to avoid the kernel corrupting a flash filesystem by using an incorrect\r\n\
	default partition layout.\r\n\
\r\n\
ARGUMENTS\r\n\
	lengthN - Size of the partition, in blocks.\r\n\
\r\n\
USAGE\r\n\
	createfis 1 2 8 116\r\n\
"
},

{
"decode",
"\
NAME\r\n\
	decode - Decode the WinCE image.\r\n\
\r\n\
SYNOPSIS\r\n\
	decode\r\n\
\r\n\
DESCRIPTION\r\n\
	Decodes the Windows CE kernel .bin image in flash and copy it\r\n\
	into RAM.\r\n\
\r\n\
USAGE\r\n\
	decode\r\n\
"

},

{
"download",
"\
NAME\r\n\
	download - Download an image by TFTP or serial.\r\n\
\r\n\
SYNOPSIS\r\n\
	download tftp[:<ip>] <filename> <destination>\r\n\
	download tftpd <destination>\r\n\
	download serial <destination>\r\n\
	download xmodem <destination>\r\n\
	download raw <destination> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	Gets a remote file from a specified source into a local memory\r\n\
	address. Use TFTP for Linux and TFTPD for WinCE (Platform Builder or\r\n\
	Eshell). In TFTPD mode, a remote TFTP client can send files to\r\n\
	port 980.  An exclamation mark means that the TFTP download failed.\r\n\
	Use serial to download over the serial port as a uuencoded text\r\n\
	file, raw to download over the serial port as a raw binary file,\r\n\
	and xmodem to download over the serial port as a raw binary file\r\n\
	using the XMODEM-CRC or 1k-XMODEM file transfer protocol.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - IP address of TFTP server from which to get image.\r\n\
	filename - Name of image on TFTP server.\r\n\
	destination - Memory address to store image.\r\n\
	length - Number of bytes to download.\r\n\
\r\n\
USAGE\r\n\
	download tftp:192.168.1.13 zImage 0xc0008000\r\n\
	download serial 0xc0020000\r\n\
	download raw 0xc0020000 46476\r\n\
"

},

#ifdef TAGGED_EEPROM
{
"eeclear",
"\
NAME\r\n\
	eeclear - Clear the contents of the EEPROM.\r\n\
\r\n\
SYNOPSIS\r\n\
	eeclear\r\n\
\r\n\
DESCRIPTION\r\n\
	Clears the contents of the EEPROM, except for the MAC address(es).\r\n\
"

},

{
"eedump",
"\
NAME\r\n\
	eedump - Displays the contents of the EEPROM.\r\n\
\r\n\
SYNOPSIS\r\n\
	eedump\r\n\
\r\n\
DESCRIPTION\r\n\
	Displays the tags stored in the EEPROM and their associated data.\r\n\
"

},
#endif // TAGGED_EEPROM

{
"eraseflash",
"\
NAME\r\n\
	eraseflash - Erases flash memory.\r\n\
\r\n\
SYNOPSIS\r\n\
	eraseflash <startblk> [<length>]\r\n\
\r\n\
DESCRIPTION\r\n\
	eraseflash will erase flash memory starting with the specified block.\r\n\
	If <length> is not specified, then eraseflash will erase until the\r\n\
	end of the flash memory on the device; you will be prompted to confirm\r\n\
	before it proceeds.\r\n\
\r\n\
ARGUMENTS\r\n\
	startblk - Flash block to start erasing at.\r\n\
	length - Number of blocks to erase.\r\n\
\r\n\
USAGE\r\n\
	eraseflash 3\r\n\
"

},

{
"exec",
"\
NAME\r\n\
	exec - Execute I-Boot script.\r\n\
\r\n\
SYNOPSIS\r\n\
	exec tftp[:<ip>] <filename>\r\n\
	exec <address>\r\n\
\r\n\
DESCRIPTION\r\n\
	Executes a script after retrieving it from a TFTP server, if\r\n\
	requested.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - IP address of TFTP server.\r\n\
	filename - Script on TFTP server to retrieve and execute.\r\n\
	address - Memory address of script.\r\n\
\r\n\
USAGE\r\n\
	exec tftp:192.168.1.13 parttest.txt\r\n\
"

},

{
"flash",
"\
NAME\r\n\
	flash - Write data from RAM into flash memory.\r\n\
\r\n\
SYNOPSIS\r\n\
	flash <destination> <source> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	flash requires a memory location in flash memory to be specified\r\n\
	at <destination> and a memory location in RAM as <source>. <length>\r\n\
	specifies how many bytes of data should be written to flash.\r\n\
\r\n\
ARGUMENTS\r\n\
	destination - Flash address.\r\n\
	source - RAM address.\r\n\
	length - Number of bytes to copy to flash.\r\n\
\r\n\
USAGE\r\n\
	flash 0x2000 0xc0020000 0x20000\r\n\
"

},

{
"flashloader",
"\
NAME\r\n\
	flashloader - Write data from RAM into flash memory.\r\n\
\r\n\
SYNOPSIS\r\n\
	flashloader <destination> <source> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	flashloader requires a memory location in flash memory to be specified\r\n\
	at <destination> and a memory location in RAM as <source>. <length>\r\n\
	specifies how many bytes of data should be written to flash.\r\n\
	flashloader is used to write to the first two blocks of flash.\r\n\
\r\n\
	CAUTION: flashloader overwrites the bootloader block of the\r\n\
	flash. Improper use can render the device unusable.\r\n\
\r\n\
ARGUMENTS\r\n\
	destination - Flash address.\r\n\
	source - RAM address.\r\n\
	length - Number of bytes to copy to flash.\r\n\
\r\n\
USAGE\r\n\
	flash 0x2000 0xc0020000 0x20000\r\n\
"

},

{
"flashverify",
"\
NAME\r\n\
	flashverify - Write data from RAM into flash memory and verify.\r\n\
\r\n\
SYNOPSIS\r\n\
	flashverify <destination> <source> <length>\r\n\
\r\n\
DESCRIPTION\r\n\
	flashverify requires a memory location in flash memory to be specified\r\n\
	at <destination> and a memory location in RAM as <source>. <length>\r\n\
	specifies how many bytes of data should be written to flash.\r\n\
	Verifies that data has been correctly written.\r\n\
\r\n\
ARGUMENTS\r\n\
	destination - Flash address.\r\n\
	source - RAM address.\r\n\
	length - Number of bytes to copy to flash.\r\n\
\r\n\
USAGE\r\n\
	flashverify 0x2000 0xc0020000 0x20000\r\n\
"
},

{
"getbyte",
"\
NAME\r\n\
	getbyte - Read byte(s) at specified address.\r\n\
	getword - Read word(s) at specified address.\r\n\
	getdword - Read double word(s) at specified address.\r\n\
\r\n\
SYNOPSIS\r\n\
	getbyte <address> [<length>]\r\n\
	getword <address> [<length>]\r\n\
	getdword <address> [<length>]\r\n\
\r\n\
DESCRIPTION\r\n\
	Reads from the specified <address>. If <length> is specified, a\r\n\
	range of addresses will be dumped to the display.\r\n\
\r\n\
ARGUMENTS\r\n\
	address - Memory location to read from.\r\n\
	length - Number of values to read.\r\n\
\r\n\
USAGE\r\n\
	getdword 0x80000004\r\n\
	getbyte 0xc0002000 0x100\r\n\
"

},

{
"help",
"\
NAME\r\n\
	help - Show usage for commands.\r\n\
\r\n\
SYNOPSIS\r\n\
	help <command>\r\n\
\r\n\
DESCRIPTION\r\n\
	Shows the usage for I-Boot commands. The list of all the\r\n\
	supported commands will be displayed if <command> is not specified.\r\n\
\r\n\
ARGUMENTS\r\n\
	command - User command to show help for.\r\n\
"

},

{
"info",
"\
NAME\r\n\
	info - Display information about I-Boot.\r\n\
\r\n\
SYNOPSIS\r\n\
	info\r\n\
\r\n\
DESCRIPTION\r\n\
	Displays important information concerning the device.\r\n\
"
},

{
"jump",
"\
NAME\r\n\
	jump - Start running a program in memory.\r\n\
\r\n\
SYNOPSIS\r\n\
	jump <start> [<arg> ...]\r\n\
\r\n\
DESCRIPTION\r\n\
	Jumps to a program loaded somewhere in memory.\r\n\
\r\n\
ARGUMENTS\r\n\
	start - Address of program.\r\n\
	arg - Numeric argument passed into program.\r\n\
\r\n\
USAGE\r\n\
	jump 0xc0008000 0 31 0\r\n\
"
},

{
"memtest",
"\
NAME\r\n\
	memtest - Test RAM.\r\n\
\r\n\
SYNOPSIS\r\n\
	memtest <start> <end>\r\n\
\r\n\
DESCRIPTION\r\n\
	Tests RAM from the <start> to the <end> address.\r\n\
\r\n\
ARGUMENTS\r\n\
	start - Base address to test.\r\n\
	end - Last address to test.\r\n\
\r\n\
USAGE\r\n\
	memtest 0xc0000000 0xc01F8000\r\n\
"
},

{
"ping",
"\
NAME\r\n\
	ping - Determine if a remote host is reachable.\r\n\
\r\n\
SYNOPSIS\r\n\
	ping <ip>\r\n\
\r\n\
DESCRIPTION\r\n\
	Sends an ICMP echo request message to a remote host.  If the host is\r\n\
	up and reachable, it should send an echo response message which is\r\n\
	displayed when it is received.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - Valid IP address.\r\n\
\r\n\
USAGE\r\n\
	ping 192.168.1.123\r\n\
"

},

{
"pcmcia",
"\
NAME\r\n\
	pcmcia - Load kernel and/or ramdisk from PCMCIA/CF card.\r\n\
\r\n\
SYNOPSIS\r\n\
	pcmcia <insert | eject>\r\n\
\r\n\
DESCRIPTION\r\n\
	Insert PCMCIA/CF card to load kernel and/or ramdisk to RAM.\r\n
	Eject PCMCAI/CF card.\r\n\
\r\n\
ARGUMENTS\r\n\
	insert - Insert PCMCIA/CF card.\r\n\
	eject - Eject PCMCIA/CF card.\r\n\
"
},

{
"reboot",
"\
NAME\r\n\
        reboot - Reboots the machine.\r\n\
\r\n\
SYNOPSIS\r\n\
	reboot\r\n\
\r\n\
DESCRIPTION\r\n\
	Restarts the machine.\r\n\
"
},

{
"runce",
"\
NAME\r\n\
        runce - Downloads and runs a WinCE .bin file.\r\n\
\r\n\
SYNOPSIS\r\n\
	runce\r\n\
\r\n\
DESCRIPTION\r\n\
	Downloads and starts a WinCE .bin file. The .bin file is stored in\r\n\
	RAM, and is lost on reboot.\r\n\
"
},

{
"set",
"\
NAME\r\n\
	set - Change settings of attributes.\r\n\
\r\n\
SYNOPSIS\r\n\
	set [<attribute> [<value>]]\r\n\
\r\n\
DESCRIPTION\r\n\
	Adjusts the settings of an attribute. To get a list of all the\r\n\
	settable attributes, run set without any arguments. To get the current\r\n\
	value for a particular attribute, type\r\n\
		set <attribute>\r\n\
\r\n\
	To get help for a particular attribute, type\r\n\
		help set <attribute>\r\n\
\r\n\
ARGUMENTS\r\n\
	attribute - Setting to change.\r\n\
	value - New value to set attribute to.\r\n\
"

},

{
"set gw",
"\
NAME\r\n\
	set gw - Set the gateway address used by the device.\r\n\
\r\n\
SYNOPSIS\r\n\
	set gw <ip>\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the network gateway address used by the device.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - Gateway IP address.\r\n\
\r\n\
USAGE\r\n\
	set gw 192.168.1.1\r\n\
"

},

{
"set ip",
"\
NAME\r\n\
	set ip - Set the static IP address of the device.\r\n\
\r\n\
SYNOPSIS\r\n\
	set ip [<ip> | dhcp | bootp]\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the static IP address for the device, or obtains one via DHCP\r\n\
	or BOOTP.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - Valid IP address.\r\n\
\r\n\
USAGE\r\n\
	set ip 192.168.1.123\r\n\
"

},

{
"set mac",
"\
NAME\r\n\
	set mac - Set the MAC address of the Ethernet device.\r\n\
\r\n\
SYNOPSIS\r\n\
	set mac [<mac> [<num>]]\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the MAC address of the Ethernet device; you will be prompted\r\n\
	to confirm before it proceeds. This command will normally never\r\n\
	be needed.\r\n\
\r\n\
	CAUTION: You can cause network problems if you set an invalid\r\n\
	MAC address.\r\n\
\r\n\
ARGUMENTS\r\n\
	mac - MAC address\r\n\
	num - 0 or 1, if the hardware supports two Ethernet ports\r\n\
\r\n\
USAGE\r\n\
	set mac 000103123456\r\n\
"

},

{
"set mask",
"\
NAME\r\n\
	set mask - Set the subnet mask of the device.\r\n\
\r\n\
SYNOPSIS\r\n\
	set mask [<mask>]\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the subnet mask of the device.\r\n\
\r\n\
ARGUMENTS\r\n\
	mask - Subnet mask.\r\n\
\r\n\
USAGE\r\n\
	set mask 255.255.255.0\r\n\
"

},

{
"set server",
"\
NAME\r\n\
	set server - Set the server address.\r\n\
\r\n\
SYNOPSIS\r\n\
	set server [<ip>]\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the default server address used by the device.\r\n\
\r\n\
ARGUMENTS\r\n\
	ip - Server IP address.\r\n\
\r\n\
USAGE\r\n\
	set server 192.168.0.1\r\n\
"

},

{
"set speed",
"\
NAME\r\n\
	set speed - Set the CPU speed.\r\n\
\r\n\
SYNOPSIS\r\n\
	set speed <speed>\r\n\
\r\n\
DESCRIPTION\r\n\
	Sets the CPU's clock frequency to the given speed in Mhz. If the\r\n\
	speed given not supported, the next lowest allowable speed is\r\n\
	set instead.\r\n\
\r\n\
ARGUMENTS\r\n\
	speed - CPU clock speed in MHz.\r\n\
\r\n\
USAGE\r\n\
	set speed 200\r\n\
"

},

{
"setbyte",
"\
NAME\r\n\
	setbyte - Write byte(s) at specified address.\r\n\
	setword - Write word(s) at specified address.\r\n\
	setdword - Write double word(s) at specified address.\r\n\
\r\n\
SYNOPSIS\r\n\
	setbyte <address> <value> [<length>]\r\n\
	setword <address> <value> [<length>]\r\n\
	setdword <address> <value> [<length>]\r\n\
\r\n\
DESCRIPTION\r\n\
	Write specified <value> of the specified size at the\r\n\
	<address>. If <length> is specified, then the\r\n\
	specified <value> will be filled to the specified <length>.\r\n\
\r\n\
ARGUMENTS\r\n\
	address - Memory location to read from.\r\n\
	value - Byte, word or double word value.\r\n\
	length - Number of values to read.\r\n\
\r\n\
USAGE\r\n\
	setdword 0xc0030000 0xffff 0x100\r\n\
"

},

{
"updatece",
"\
NAME\r\n\
	updatece - Downloads and flashes a WinCE .bin file.\r\n\
\r\n\
SYNOPSIS\r\n\
	updatece\r\n\
\r\n\
DESCRIPTION\r\n\
	Downloads and flashes a WinCE .bin file. The .bin file can\r\n\
	be started with the boot command.\r\n\
"
},

{NULL,
"No help available for that command\r\n"
}
};


#ifndef DUMP_HELP

static inline char const *
find_help(char const *cmd)
{
    int i = 0;

    while (helpText[i].cmd != NULL)
    {
        int offset = 0;
        char const *cur_page = helpText[i].cmd;

        /* Set and get have a second argument, so ignore extra whitespace */
        if (cmpstr(cmd, "set") && *(cmd + sizeof("set") - 1) == ' ')
        {
            offset = (next_token(cmd) - cmd);
            cur_page = next_token(cur_page);
        }
        if (cmpstr(cmd + offset, cur_page))
        {
            break;              // found the command
        }
        else
        {
            i++;
        }
    }
    return helpText[i].helpText;
}

void
print_help(char const *arg)
{
    while (*arg && *arg <= ' ')
        arg++;                  //skip leading whitespace

    if (!*arg)
    {
        list_commands();
        return;
    }

//special cases for setbyte/setword/setdword and getbyte/getword/getdword.
    if (itc_strcmp("set", arg) &&
        *(arg + sizeof("set") - 1) != ' ' &&
        *(arg + sizeof("set") - 1) != '\0')
    {
        arg = "setbyte";
    }
    else if (itc_strcmp("get", arg) &&
             *(arg + sizeof("get") - 1) != ' ' &&
             *(arg + sizeof("get") - 1) != '\0')
    {
        arg = "getbyte";
    }

    itc_printf("%s", find_help(arg));
}

#else

/*
 * Dump all help text to stdout so it can be placed into a text file.
 * Compile with:
 *   cc -Wall -DDUMP_HELP -DTAGGED_EEPROM -I../../include -o help help.c
 */

#include <stdio.h>
#include <config.h>
int
main(void)
{
    THelpText const *help;

    printf("I-Boot Command Reference\r\n");
    printf("Copyright 2001-2002 Intrinsyc Software Inc.\r\n");
    for (help = helpText; help->cmd; ++help)
    {
        printf("\r\n----------------\r\n");
        printf("%s", help->helpText);
    }
    return 0;
}
#endif // DUMP_HELP
