////////////////////////////////////////////////////////////////////////////////
//
// Copyright(c) 2001 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      parser.c
//
// Description:
//
//      Handles the parsing, and execution of, incoming text commands.
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

#include <parser.h>
#include <config.h>
#include <serial.h>
#include <platform.h>
#include <flash_i128.h>
#include <c_main.h>
#include <string.h>
#include <cedecode.h>
#include <string.h>
#include <cebootme.h>
#include <tftp.h>
#include <params.h>
#include <util.h>
#include <ethernet.h>
#include <memtest.h>
#include <uudecode.h>
#include <bootp.h>
#include <help.h>
#include <net.h>
#include <dhcp.h>
#include <os.h>
#include <messages.h>
#include <checksum.h>
#include <timer.h>
#include <xmodem.h>
#include <icmp.h>
#include <fis.h>
#include <eeprom.h>
#include <nvconfig.h>
#include <idle.h>
#include <cpu.h>
#include <pcmcia.h>

int boot_parse(char const * arg);
int bootk_parse(char const *arg);
int bootme_parse(char const * arg);
int bootmem_parse(char const * arg);
int jump_parse(char const * arg);
int copy_parse(char const * arg);
int createfis_parse(char const * arg);
int decode_parse(char const * arg);
int download_parse(char const *arg);
int eeclear_parse(char const * arg);
int eedump_parse(char const * arg);
int eraseflash_parse(char const * arg);
int exec_parse(char const *);
int flash_parse(char const *arg);
int flashloader_parse(char const *arg);
int flashverify_parse(char const * arg);
int getbyte_parse(char const * arg);
int getword_parse(char const * arg);
int getdword_parse(char const * arg);
int help_parse(char const * arg);
int info_parse(char const * arg);
int memtest_parse(char const * arg);
int ping_parse(char const * arg);
int pcmcia_parse(char const * arg);
int updatece_parse(char const * arg);
int reboot_parse(char const * arg);
int runce_parse(char const * arg);
int setbyte_parse(char const * arg);
int setword_parse(char const * arg);
int setdword_parse(char const * arg);
int set_parse(char const *arg);
int set_ip_parse(char const *arg);
int set_mac_parse(char const *arg);
int set_server_parse(char const *arg);
int set_speed_parse(char const *arg);
int set_mask_parse(char const *arg);
int set_gw_parse(char const *arg);
int crc_parse(char const *arg);

command_def command_set[] =
{
   // command       function_name       min. # parameters
   { "boot",        boot_parse,         0 },
   { "bootk",       bootk_parse,        1 },
   { "bootme",      bootme_parse,       0 },
   { "bootmem",     bootmem_parse,      2 },
   { "copy",        copy_parse,         3 },
   { "crc",         crc_parse,          2 },
   { "createfis",   createfis_parse,    1 },
   { "decode",      decode_parse,       0 },
   { "download",    download_parse,     2 },
#ifdef TAGGED_EEPROM
   { "eeclear",     eeclear_parse,      0 },
   { "eedump",      eedump_parse,       0 },
#endif // TAGGED_EEPROM
   { "eraseflash",  eraseflash_parse,   1 },
   { "exec",        exec_parse,         1 },
   { "flash",       flash_parse,        3 },
   { "flashloader", flashloader_parse,  3 },
   { "flashverify", flashverify_parse,  3 },
   { "getbyte",     getbyte_parse,      1 },
   { "getword",     getword_parse,      1 },
   { "getdword",    getdword_parse,     1 },
   { "help",        help_parse,         0 },
   { "info",        info_parse,         0 },
   { "jump",        jump_parse,         1 },
   { "memtest",     memtest_parse,      2 },
   { "ping",        ping_parse,         1 },
   { "pcmcia",		pcmcia_parse,		1 },
   { "reboot",      reboot_parse,       0 },
   { "runce",       runce_parse,        0 },
   { "setbyte",     setbyte_parse,      2 },
   { "setword",     setword_parse,      2 },
   { "setdword",    setdword_parse,     2 },
   { "set",         set_parse,          0 },
   { "set gw",      set_gw_parse,       1 },
   { "set ip",      set_ip_parse,       1 },
   { "set mac",     set_mac_parse,      1 },
   { "set mask",    set_mask_parse,     1 },
   { "set server",  set_server_parse,   1 },
   { "set speed",   set_speed_parse,    2 },
   { "updatece",    updatece_parse,     0 },
};

////////////////////////////////////////////////////////////////////////////////
// list_commands
// PURPOSE: List all the commands known to the parser
// PARAMS:  None.
// RETURNS: None.
////////////////////////////////////////////////////////////////////////////////
void
list_commands(void)
{
   int i;

#define COMMANDS_PER_LINE 7
   for(i = 0; i < countof(command_set); ++i)
   {
      if ((i % COMMANDS_PER_LINE) == 0)
      {
         if (i)
         {
            /* End of line */
            itc_printf(",\r\n");
         }
      }
      else
      {
         itc_printf(", ");
      }
      itc_printf("%s",command_set[i].command);
   }
   itc_printf("\r\n");
}

////////////////////////////////////////////////////////////////////////////////
// get_token_count
// PURPOSE: Returns the number of tokens in a given string.
// PARAMS:  (IN) char *arg - string to count.
// RETURNS: Number of tokens in arg.
////////////////////////////////////////////////////////////////////////////////
static int
get_token_count(char const *arg)
{
   int retval = 0;

   while(*arg != '\0')
   {
      arg = next_token(arg);
      retval++;
   }

   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// parse_command
// PURPOSE: Parses the command and passes its arguments on to the appropriate
//          handler function.
// PARAMS:  (IN) char *commandline - command to parse.
// RETURNS: mode - the current mode. Currently partition or default.
////////////////////////////////////////////////////////////////////////////////
mode
parse_command(char const *commandline,
              mode cur_mode)
{
   int i = 0;

   while (*commandline && *commandline <= ' ') commandline++;  //skip leading whitespace
   if(!*commandline || *commandline == '#')
   {
      return cur_mode;
   }

   for(i = countof(command_set); i >= 0; i--)
   {
      if(cmpstr(commandline, command_set[i].command))
      {
	 if((get_token_count(commandline) - 1) >= command_set[i].min_args)
	 {
	    //Call the function pointer in the appropriate command_def, and
	    //return the current mode on success, or mode_error on failure.
	    char const *args = commandline + itc_strlen(command_set[i].command);
   	    while (*args && *args <= ' ') args++;  //skip whitespace after command name
	    return ((command_set[i].parse_func) (args) ? cur_mode : mode_error);
	 }
	 else
	 {
	    error_print(PARSE_MIN_ARGS_ERROR);
	    return mode_error;
	 }
      }
   }

   error_print(PARSE_NO_COMMAND_ERROR);
   return mode_error;
}

////////////////////////////////////////////////////////////////////////////////
// parse and execute a script
// PURPOSE: Parses a script in memory.
// PARAMS:  (IN) char *curline - script to parse terminated by '\0'.
// RETURNS: mode - the current mode, currently partition or default.
////////////////////////////////////////////////////////////////////////////////
mode
parse_script(char const *script)
{
    mode curmode = mode_default;
    char const *curpos = script;
    unsigned len;
    char cmdbuf[256];           //holds only one line at a time

    while (*script)
    {
        while (*curpos && *curpos != '\r' && *curpos != '\n')
        {
            curpos++;
        }
        len = curpos - script;
        if (len >= sizeof(cmdbuf))
        {
            itc_printf("Script line too long!\r\n");
            return mode_error;
        }
        itc_strlcpy(cmdbuf, script, len+1);        //copy script line to RAM
        itc_printf("IBoot: %s\r\n", cmdbuf);
        curmode = parse_command(cmdbuf, curmode);
        if (curmode == mode_error)
        {
            break;
        }

        // skip over Unix (or DOS) line break
        if (curpos[0] == '\r' && curpos[1] == '\n')
        {
            curpos++;
        }
        if (*curpos)
        {
            curpos++;
        }
        script = curpos;
    }
    return curmode;
}

////////////////////////////////////////////////////////////////////////////////
// get_number_parse
// PURPOSE: Parses an address an address or other 32 bit hexadecimal number.
// PARAMS:  (IN)  char *arg    - string to parse.
//          (OUT) u8 **address - address parsed.
// RETURNS: 0 on failure, address of next token in arg on success. If there are
//          no more tokens, return value will point to the strings null.
////////////////////////////////////////////////////////////////////////////////
static char const
*get_number_parse(char const *arg, u32 *address)
{
   int offset;

   if(*arg == '0' && *(arg + 1) == 'x')
   {
      arg += 2;
      offset = atou32(arg, (u32 *)address);
   }
   else
      offset = atoi(arg, (int *)address);

   return (offset ? next_token(arg + offset) : (char *)offset);
}

////////////////////////////////////////////////////////////////////////////////
// get_filename_parse
// PURPOSE: Parses out a filename from a string.
// PARAMS:  (IN) char *arg - argument(s) to the download command.
//          (OUT) char *buf - buffer to hold file name
//          (IN) int size - size of buffer
// RETURNS: 0 on failure, address of next token in arg on success. If there are
//          no more tokens, return value will point to the strings null.
////////////////////////////////////////////////////////////////////////////////
static char const
*get_filename_parse(char const *arg,
                    char *buf,
                    int size)
{
   int i = 0;

   while(*arg != ' ' && *arg != 0 && i < size)
   {
      if (*arg == '\\' && *(arg+1))
      {
         // skip over backslash and take next char unconditionally
         ++arg;
      }
      *buf++ = *arg++;
      i++;
   }
   *buf = 0;

   return (i ? next_token(arg) : (char *)0);
}

////////////////////////////////////////////////////////////////////////////////
// download_parse:
// PURPOSE: Parses a download command and executes appropriate download method.
// PARAMS:  (IN) char *arg - argument(s) to the download command.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
download_parse(char const *arg)
{
   enum
   {
      unknown,
      tftp,
      tftpd,
      serial,
      raw,
      xmodem
   } dl_type;

   char *dest = 0;
   char filename[MAX_FILENAME_SIZE];
   u32 ip = 0;
   int len;

   if(cmpstr(arg, "tftp"))
   {
      dl_type = tftp;
   }
   else if(cmpstr(arg, "tftpd"))
   {
      dl_type = tftpd;
   }
   else if(cmpstr(arg, "serial"))
   {
      dl_type = serial;
   }
   else if(cmpstr(arg, "raw"))
   {
      dl_type = raw;
   }
   else if(cmpstr(arg, "xmodem"))
   {
      dl_type = xmodem;
   }
   else
   {
      dl_type = unknown;
   }

   arg = next_token(arg);

   if(dl_type == tftp)
   {
      // Parse IP address if specified, otherwise use default server
      if (*arg == ':')
      {
          if((ip = atoip(arg+1)))
          {
             status.siaddr = ip;
             arg = next_token(arg);
          }
          else
          {
             error_print(PARSE_INVALID_IP_ERROR);
             return 0;
          }
      }
      if (status.siaddr == 0)
      {
         error_print(PARSE_INVALID_IP_ERROR);
         return 0;
      }

      if(!(get_filename_parse(arg, filename, sizeof(filename))))
      {
         error_print(PARSE_INVALID_ARG_ERROR);
         return 0;
      }
      arg = next_token(arg);
   }

   if(!(get_number_parse(arg, (u32 *)&dest)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if((u32)dest < PLATFORM_MEMORY_BASE || (u32)dest > PLATFORM_MEMORY_MAX)
   {
      error_print(PARSE_RANGE_ERROR);
      return 0;
   }

   switch(dl_type)
   {
      case raw:
      {
         if(!(get_number_parse(arg, (u32 *)&len)))
         {
            error_print(PARSE_INVALID_ARG_ERROR);
            return 0;
         }

         if (!raw_input_serial(dest, len, 3600*100))
         {
	    itc_printf("Timeout in serial transfer\r\n");
	    return 0;
         }
         break;
      }
      case serial:
      {
         len = uudecode(dest);
         break;
      }
      case xmodem:
      {
         itc_printf("Start upload procedure using Xmodem-CRC Protocol.\r\n");
         itc_printf("(Press ESC to cancel.)\r\n");

         if (Xrecv(dest, (u32 *)&len))
         {
	    itc_printf("Error in XMODEM transfer\r\n");
	    return 0;
	 }
         break;
      }
      case tftpd:
      {
	 if(status.ciaddr == 0)
	 {
	    u32 temp;
	    message_print(PARSE_DHCP_MESSAGE);
	    if(!init_dhcp(&status.ciaddr, &temp, &status.giaddr, &status.smask))
	    {
	       error_print(DHCP_TIMEOUT_ERROR);
	       return 0;
	    }
	 }
         do
            cebootme();
         while((len = tftplisten(dest)) < 0)
                ;
         break;
      }
      case tftp:
      {
         if(status.ciaddr == 0)
	 {
	    u32 temp;
	    message_print(PARSE_DHCP_MESSAGE);
	    if(!init_dhcp(&status.ciaddr, &temp, &status.giaddr, &status.smask))
	    {
	       error_print(DHCP_TIMEOUT_ERROR);
	       return 0;
	    }
	 }
         if((len = tftpget((char *)filename, dest)) < 0)
	 {
	    itc_printf("\r\n");
            error_print(TFTP_TIMEOUT_ERROR);
	    return 0;
	 }
         break;
      }
      default:
      {
         error_print(PARSE_DL_METHOD_ERROR);
	 return 0;
      }
   }

#ifdef SHOW_DOWNLOAD_CRC
   itc_printf("CRC-32 of %d bytes is 0x%x\r\n",
              len, update_crc(INITIAL_CRC32, dest, len));
#endif
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// flash_parse
// PURPOSE: Parses a flash command and writes to flash accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the flash command.
// RETURNS: 1 for success, 0 for failure.
// NOTES:   This function may not flash the boot block.
////////////////////////////////////////////////////////////////////////////////
int
flash_parse(char const *arg)
{
   u32 *dest = (u32 *)(flash_block_size_platform());
   u32 *src = 0;
   int buflen = 0;

   if(get_number_parse(arg, (u32 *)&dest))
      arg = next_token(arg);
   else
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if(get_number_parse(arg, (u32 *)&src))
      arg = next_token(arg);
   else
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if(!get_number_parse(arg, (u32 *)&buflen))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if((u32)dest % sizeof(u32))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

   if((u32)src % sizeof(u32))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

   if((u32)src < PLATFORM_MEMORY_BASE ||
      (u32)src >= PLATFORM_MEMORY_MAX ||
      (u32)dest < PLATFORM_FLASH_BASE ||
      (u32)dest >= (PLATFORM_FLASH_BASE + PLATFORM_FLASH_SIZE) ||
      ((u32)dest + buflen) > (PLATFORM_FLASH_BASE+PLATFORM_FLASH_SIZE))
   {
      error_print(PARSE_RANGE_ERROR);
      return 0;
   }

   itc_printf("Flashing: ");
   return block_write_flash(dest, src, buflen, FLASH_DEFAULT);
}

////////////////////////////////////////////////////////////////////////////////
// flashverify_parse
// PURPOSE: Parses a flash command and writes to flash accordingly, with a
//          verify after.
// PARAMS:  (IN) char *arg - argument(s) to the flash command.
// RETURNS: 1 for success, 0 for failure.
// NOTES:   This function may not flash the boot block.
////////////////////////////////////////////////////////////////////////////////
int
flashverify_parse(char const *arg)
{
   int retval;

   retval = flash_parse(arg);

   if(retval)
   {
      u32 *dest;
      u32 *src;
      u32 length;

      arg = get_number_parse(arg, (u32 *)&dest);
      arg = get_number_parse(arg, (u32 *)&src);
      arg = get_number_parse(arg, &length);

      if(cmp32(dest, src, (int)length / sizeof(u32)))
      {
	 return 1;
      }
      else
      {
	 error_print(FLASH_VERIFY_ERROR);
	 return 0;
      }
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// flashloader_parse
// PURPOSE: Parses a flash command and writes to flash accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the flash command.
// RETURNS: 1 for success, 0 for failure.
// NOTES:   This function may flash the boot block.
////////////////////////////////////////////////////////////////////////////////
int
flashloader_parse(char const *arg)
{
   u32 *dest = (u32 *)(flash_block_size_platform());
   u32 *src = 0;
   int buflen = 0;

   if(get_number_parse(arg, (u32 *)&dest))
      arg = next_token(arg);
   else
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if(get_number_parse(arg, (u32 *)&src))
      arg = next_token(arg);
   else
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if(!get_number_parse(arg, (u32 *)&buflen))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if((u32)dest % sizeof(u32))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

   if((u32)src % sizeof(u32))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

  if((u32)src < PLATFORM_MEMORY_BASE ||
     (u32)src >= PLATFORM_MEMORY_MAX ||
     (u32)dest < PLATFORM_FLASH_BASE ||
     (u32)dest >= (PLATFORM_FLASH_BASE + PLATFORM_FLASH_SIZE) ||
     ((u32)dest + buflen) > (PLATFORM_FLASH_BASE+PLATFORM_FLASH_SIZE))
   {
      error_print(PARSE_RANGE_ERROR);
      return 0;
   }

   itc_printf("Flashing: ");
   return block_write_flash(dest, src, buflen, FLASH_BOOT);
}

////////////////////////////////////////////////////////////////////////////////
// kernel_param_parse
// PURPOSE: Fixes a quote-mangled kernel parameter string.
// PARAMS:  (IN)  char *mangled - Mangled string to fix.
//          (OUT) char *buf     - buffer to write fixed string into
//          (IN) int bufsize    - length of buffer
// RETURNS: Number of characters in fixed string, or -1 for error.
////////////////////////////////////////////////////////////////////////////////
int
kernel_param_parse(char const *mangled, char *buf, int bufsize)
{
   int retval = 0;

   if (*mangled) {  /* allow missing parameter string */
      if(*mangled++ != '\"')
      {
         error_print(PARSE_KERN_PARAM_ERROR);
         return -1;
      }

      while(*mangled != 0 && *mangled != '\"' && retval < (bufsize-1))
      {
         if(*mangled == '\\' && *(mangled + 1) == '\"')
            mangled++;

         *buf++ = *mangled++;
         retval++;
      }

      if(*mangled != '\"')
      {
         error_print(PARSE_KERN_PARAM_ERROR);
         return -1;
      }
   }

   *buf = 0;

   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// bootmem_parse
// PURPOSE: Parses a bootmem command and boots from the appropriate area in
//          memory. Handles any OS specific things (such as CE BIN parsing)
//          aswell.
// PARAMS:  char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
bootmem_parse(char const *arg)
{
   enum
   {
      unknown,
      linux_kernel,
      ce
   } os;

   u32 addr;

   if(cmpstr(arg, "linux"))
   {
      os = linux_kernel;
   }
   else if(cmpstr(arg, "ce"))
   {
      os = ce;
   }
   else
   {
      os = unknown;
   }

   arg = next_token(arg);
   if(!get_number_parse(arg, &addr))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   switch(os)
   {
      case linux_kernel:
      {
         boot_linux_kernel( arg, (u8 *)addr);
         break;
      }
      case ce:
      {
         void (*kernel)(void);

         message_print(RAM_CLEAR_MESSAGE);
         memset32((u32 *)PLATFORM_MEMORY_BASE, 0, CE_CLEAR_SIZE / 4);

         message_print(LOADING_CE_MESSAGE);
         kernel = (void (*)(void))cedecode((u8 *)CE_RAM_BASE, (u8 *)addr);

         if((u32 *)kernel != NULL)
            kernel();
         break;
      }
      default:
      {
         error_print(PARSE_INVALID_OS_ERROR);
      }
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// jump_parse
// PURPOSE: Jumps to a location in memory.
// PARAMS:  char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
jump_parse(char const *arg)
{
   u32 addr;
   u32 args[4];
   int rc,i;
   int (*func)(u32 a, u32 b, u32 c, u32 d);

   if(!get_number_parse(arg, &addr))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   for (i=0; i < countof(args); ++i)
   {
       if (*arg)
       {
           if(!get_number_parse(arg, &args[i]))
           {
              error_print(PARSE_INVALID_ARG_ERROR);
              return 0;
           }
           arg = next_token(arg);
       }
       else
       {
           args[i] = 0;
       }
   }

   if (*arg)
   {
       itc_printf("Too many arguments.\r\n");
       return 0;
   }

   func = (int (*)(u32 a, u32 b, u32 c, u32 d)) addr;
   rc = func(args[0], args[1], args[2], args[3]);
   itc_printf("Program returned %d.\r\n", rc);
   return rc;
}

////////////////////////////////////////////////////////////////////////////////
// boot_parse
// PURPOSE: Boots the default partition.
// PARAMS:  (IN) char *arg - kernel config string.
// RETURNS: Never returns on success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
boot_parse(char const *arg)
{
   run_os(arg); // src/libs/base/os.c
   return 0;
}

int
bootk_parse(char const *arg)
{
   int i;

   itc_strcpy(kname,arg);

   if(pcmcia(1)==0) {
      bootmem_parse("linux 0xa0008000");
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// decode_parse
// PURPOSE: Parses a decode command and decodes into memory appropriately. Only
//          CE needs this at the moment.
// PARAMS:  (IN) char *arg - argument(s) to the decode command. (None)
// NOTES:   This is obsolete, and should be removed.
// RETURNS: CE kernel address for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
decode_parse(char const *arg)
{
   message_print(LOADING_CE_MESSAGE);
   status.CEKernel = cedecode((u8 *)CE_RAM_BASE, (u8 *)CE_TEMP_RAM);
   status.os = CE;

   return (int)status.CEKernel;
}

////////////////////////////////////////////////////////////////////////////////
// info_parse
// PURPOSE: Parses an info command and prints the info table.
// PARAMS:  (IN) char *arg - argument(s) to the status command. Unused for the
//                           moment.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
info_parse(char const *arg)
{
   char temp[5];
   unsigned short mac[3];

   itc_printf("IBoot version " VERSION "\r\n");
   itc_printf("%s\r\n", copyright);
   itc_printf("Built on " __DATE__ " at " __TIME__ "\r\n");
   itc_printf("Current IP:                    %s\r\n", iptoa(status.ciaddr));
   itc_printf("Current Server IP:             %s\r\n", iptoa(status.siaddr));
   itc_printf("Current Gateway:               %s\r\n", iptoa(status.giaddr));
   itc_printf("Current Subnet Mask:           %s\r\n", iptoa(status.smask));

/*
   itc_printf("MAC Address in use:            ");
   u16toa(htons(status.macaddr[0]), temp);
   itc_printf(temp);
   u16toa(htons(status.macaddr[1]), temp);
   itc_printf(temp);
   u16toa(htons(status.macaddr[2]), temp);
   itc_printf(temp);
   itc_printf("\r\n");
*/
   if (read_mac_ethernet (mac, 0))
   {
       itc_printf("MAC Address 0:                 ");
       u16toa(htons(mac[0]), temp);
       itc_printf(temp);
       u16toa(htons(mac[1]), temp);
       itc_printf(temp);
       u16toa(htons(mac[2]), temp);
       itc_printf(temp);
       itc_printf("\r\n");
   } else
   {
       itc_printf("Warning: No MAC address was found; set it now\r\n");
   }

   if (read_mac_ethernet (mac, 1))
   {
       itc_printf("MAC Address 1:                 ");
       u16toa(htons(mac[0]), temp);
       itc_printf(temp);
       u16toa(htons(mac[1]), temp);
       itc_printf(temp);
       u16toa(htons(mac[2]), temp);
       itc_printf(temp);
       itc_printf("\r\n");
   }

   itc_printf("FLASH config:                  ");
   if(status.flash_chip == i128x2)
   {
      itc_printf("32 bit interleaved\r\n");
   }
   else if(status.flash_chip == i128)
   {
      itc_printf("16 bit non-interleaved\r\n");
   }

   itc_printf("FLASH blocks:                  %d x 0x%x (%d MB)\r\n",
              flash_size_platform() / flash_block_size_platform(),
              flash_block_size_platform(),
              (flash_size_platform() / 1024) / 1024);
   itc_printf("RAM size:                      %d MB @ 0x%x\r\n",
              (mem_size_platform() / 1024) / 1024, mem_base_platform());

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// set_parse
// PURPOSE: Prints a messege telling the user what attributes are available
//          via the set command.
// PARAMS:  (IN) char *arg - argument(s) to the set_parse command (not used).
// RETURNS: 0 (This function fails by definition, as the user has not given
//          us an attribute.)
////////////////////////////////////////////////////////////////////////////////
int
set_parse(char const *arg)
{
   message_print(PARSE_SET_VAR_MESSAGE);
   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// set_server_parse
// PURPOSE: Parses a setserver command and changes the server IP address
//          accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the setserver command.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_server_parse(char const *arg)
{
   u32 ip;
   int retval = 0;

   if(*arg != 0)
   {
      if(!(ip = atoip(arg)))
      {
	 error_print(PARSE_INVALID_ARG_ERROR);
	 retval = 0;
      }
      else
      {
	 status.siaddr = ip;
	 retval = 1;
      }
   }

   itc_printf("Server IP address: %s\r\n", iptoa(status.siaddr));
   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// set_speed_parse
// PURPOSE: Parses a set speed command and changes the CPU speed accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the set speed command.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_speed_parse(char const *arg)
{
   u32 speed;

   if(!(arg = get_number_parse(arg, &speed)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else
   {
      const int newspeed = set_cpu_speed(speed);
      if (newspeed < 0)
      {
         error_print(PARSE_INVALID_ARG_ERROR);
      } else
      {
         const u16 storedspeed = newspeed;  // Store as u16 to save space
         itc_printf("CPU speed set to %d MHz\r\n", newspeed);
         (void) eeprom_write_item("CPUCLK", sizeof("CPUCLK")-1,
                                  &storedspeed, sizeof(storedspeed));
      }
   }

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// set_gw_parse
// PURPOSE: Parses a set gw (gateway) command and changes the gateway IP address
//          accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the set gw command.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_gw_parse(char const *arg)
{
   u32 ip;
   int retval = 0;

   if(*arg != 0)
   {
      if(!(ip = atoip(arg)))
      {
	 error_print(PARSE_INVALID_ARG_ERROR);
	 retval =  0;
      }
      else
      {
	 const u32 flags = FLAG_USE_STATICIP;
	 status.giaddr = ip;
         (void) eeprom_write_item("FLAGS", sizeof("FLAGS")-1,
                                  &flags, sizeof(flags));
         (void) eeprom_write_item("SGATE", sizeof("SGATE")-1,
                                  &status.giaddr, sizeof(status.giaddr));
	 retval = 1;
      }
   }

   itc_printf("Gateway IP address: %s\r\n", iptoa(status.giaddr));
   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// set_mask_parse
// PURPOSE: Parses a set mask command and changes the subnet mask accordingly.
// PARAMS:  (IN) char *arg - argument(s) to the set mask command.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_mask_parse(char const *arg)
{
   u32 ip;
   int retval = 0;

   if(*arg != 0)
   {
      if(!(ip = atoip(arg)))
      {
	 error_print(PARSE_INVALID_ARG_ERROR);
	 retval = 0;
      }
      else
      {
         const u32 flags = FLAG_USE_STATICIP;
	 status.smask = ip;
         (void) eeprom_write_item("FLAGS", sizeof("FLAGS")-1,
                                  &flags, sizeof(flags));
         (void) eeprom_write_item("SMASK", sizeof("SMASK")-1,
                                  &status.smask, sizeof(status.smask));
	 retval = 0;
      }
   }

   itc_printf("Subnet Mask: %s\r\n", iptoa(status.smask));
   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// getbyte_parse
// PURPOSE: Parses a getbyte command and prints memory locations accordingly.
// PARAMS:  (IN) char *arg - argument string to parse.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
getbyte_parse(char const *arg)
{
   u8 *address;
   u32 length;

   if(!(arg = get_number_parse(arg, (u32 *)&address)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else if(!get_number_parse(arg, &length))
   {
      length = 1;
   }

   print_bytes(address, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// getbyte_parse
// PURPOSE: Parses a getword command and prints memory locations accordingly.
// PARAMS:  (IN) char *arg - argument string to parse.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
getword_parse(char const *arg)
{
   u16 *address;
   u32 length;

   if(!(arg = get_number_parse(arg, (u32 *)&address)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else if(!get_number_parse(arg, &length))
   {
      length = 1;
   }

   print_words(address, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// getdword_parse
// PURPOSE: Parses a getdword command and prints memory locations accordingly.
// PARAMS:  (IN) char *arg - argument string to parse.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
getdword_parse(char const *arg)
{
   u32 *address;
   u32 length;

   if(!(arg = get_number_parse(arg, (u32 *)&address)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else if(!get_number_parse(arg, &length))
   {
      length = 1;
   }

   print_dwords(address, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// setbyte_parse
// PURPOSE: Sets 8 bit aligned memory to an 8 bit pattern.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
setbyte_parse(char const *arg)
{
   u8 *address;
   u32 value;
   int length;

   if(!get_number_parse(arg, (u32 *)&address))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!(get_number_parse(arg, &value)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!get_number_parse(arg, (u32 *)&length))
   {
      length = 1;
   }

   memset8(address, (u8)value, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// setword_parse
// PURPOSE: Sets 16 bit aligned memory to a 16 bit pattern.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
setword_parse(char const *arg)
{
   u16 *address;
   u32 value;
   int length;

   if(!(get_number_parse(arg, (u32 *)&address)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!(get_number_parse(arg, &value)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!get_number_parse(arg, (u32 *)&length))
   {
      length = 1;
   }

   if((u32)address % sizeof(u16))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

   memset16(address, (u16)value, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// setdword_parse
// PURPOSE: Sets 32 bit aligned memory to an 32 bit pattern.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
setdword_parse(char const *arg)
{
   u32 *address;
   u32 value;
   int length;

   if(!(get_number_parse(arg, (u32 *)&address)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!(get_number_parse(arg, &value)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   if(!get_number_parse(arg, (u32 *)&length))
   {
      length = 1;
   }

   if((u32)address % sizeof(u32))
   {
      error_print(PARSE_ALIGN_ERROR);
      return 0;
   }

   memset32(address, value, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// exec_parse
// PURPOSE: Executes, and if necessary downloads, a script containing IBoot
//          commands
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
exec_parse(char const *arg)
{
   int size = 0;
   char *curline;
   char buf[MAX_SCRIPT_SIZE], filename[MAX_FILENAME_SIZE];
   u32 addr;

   /* see if address of script was specified */
   if(!get_number_parse(arg, (u32 *)&curline))
   {
      /* see if IP address was given */
      arg = next_token(arg);
      curline = (char *)buf;

      if (*arg == ':')
      {
         if(!(addr = atoip(arg)))
         {
            error_print(PARSE_INVALID_IP_ERROR);
            return 0;
         }
         status.siaddr = addr;
         arg = next_token(arg);
      }
      if (status.siaddr == 0)
      {
         error_print(PARSE_INVALID_IP_ERROR);
         return 0;
      }

      if(!(get_filename_parse(arg, filename, sizeof(filename))))
      {
         error_print(PARSE_INVALID_ARG_ERROR);
         return 0;
      }

      if(status.ciaddr == 0)
      {
	 u32 temp;
	 message_print(PARSE_DHCP_MESSAGE);
	 if(!init_dhcp(&status.ciaddr, &temp, &status.giaddr, &status.smask))
	 {
	    error_print(DHCP_TIMEOUT_ERROR);
	    return 0;
	 }
      }

      size = tftpget(arg, (u8 *)buf);

      if(size < 0)
      {
	 itc_printf("\r\n");
         error_print(TFTP_TIMEOUT_ERROR);
         return 0;
      }
      if(size >= sizeof(buf))
      {
         /* The stack has been overwritten so it is unsafe to continue */
	 itc_printf("\r\n");
         error_print(PARSE_INVALID_ARG_ERROR);
	 itc_printf("Halting.\r\n");

	 /* endless loop */
         for (;;);
      }
      buf[size] = 0;
   }

   return parse_script (curline) != mode_error;
}

////////////////////////////////////////////////////////////////////////////////
// copy_parse
// PURPOSE: Copies a region of memory to another region of memory.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
copy_parse(char const *arg)
{
   int length;
   u8 *dest, *src;
   char const *temp;

   temp = get_number_parse(arg, (u32 *)&dest);
   if (temp)
   {
      temp = get_number_parse(temp, (u32 *)&src);
      if (temp)
      {
         temp = get_number_parse(temp, (u32 *)&length);
      }
   }

   if(temp == NULL)
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   if((u32)dest < PLATFORM_MEMORY_BASE || (u32)dest > PLATFORM_MEMORY_MAX)
   {
      error_print(PARSE_RANGE_ERROR);
      return 0;
   }

   itc_memcpy (dest, src, length);

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// memtest_parse
// PURPOSE: Runs various memory tests on a memory range.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
memtest_parse(char const *arg)
{
   char const *temp;
   u32 start, end;

   if(!(temp = get_number_parse(arg, &start)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else if(!get_number_parse(temp, &end))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else
   {
      itc_printf("Testing from 0x%x to 0x%x\r\n", (u32)start, (u32)end);
      itc_printf("Walking ones: %s.\r\n",
             (do_walking_ones((unsigned int)start, (unsigned int)end) ?
             "Failed" : "Passed"));
      itc_printf("Walking zeros: %s.\r\n",
             (do_walking_zeros((unsigned int)start, (unsigned int)end) ?
             "Failed" : "Passed"));
      itc_printf("Streaming ones: %s.\r\n",
             (do_streaming_ones((unsigned int)start, (unsigned int)end) ?
             "Failed" : "Passed"));
      itc_printf("Streaming zeros: %s.\r\n",
             (do_streaming_zeros((unsigned int)start, (unsigned int)end) ?
             "Failed" : "Passed"));
   }

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// crc_parse
// PURPOSE: Calculates CRC on a memory range.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
crc_parse(char const *arg)
{
   char const *temp;
   u32 start, len;

   if(!(temp = get_number_parse(arg, &start)))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else if(!get_number_parse(temp, &len))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   else
   {
      itc_printf("CRC-32 of %d bytes at 0x%x is 0x%x\r\n", len, start,
                 update_crc(INITIAL_CRC32, (u8 *)start, len));
   }

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// bootme_parse
// PURPOSE: Broadcasts a MS Platform Builder "bootme" message to announce our
//          presence to any instances of such on the subnet.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
bootme_parse(char const *arg)
{
   cebootme();
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// help_parse
// PURPOSE: Parses a help command and prints the appropriate help message.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
help_parse(char const *arg)
{
   print_help(arg);
   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// updatece_parse
// PURPOSE: Parses an updatece command, downloads and flashes a CE .bin file.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
updatece_parse(char const *arg)
{
   int size = 0;

   if(status.ciaddr == 0)
   {
      message_print(PARSE_DHCP_MESSAGE);
      if(!init_dhcp(&status.ciaddr,
	            &status.siaddr,
		    &status.giaddr,
		    &status.smask))
      {
	 error_print(DHCP_TIMEOUT_ERROR);
	 return 0;
      }
   }

   do
      cebootme();
   while((size = tftplisten((u8 *)BL_TEMP_RAM)) < 0);

   itc_printf("Flashing bin file: ");

   return block_write_flash((u32 *)KERNEL_FLASH_START,
		            (u32 *)BL_TEMP_RAM,
			    size,
			    FLASH_DEFAULT);
}

////////////////////////////////////////////////////////////////////////////////
// runce_parse
// PURPOSE: Parses an updatece command, downloads, decodes, and boots a CE .bin
//          file.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
runce_parse(char const *arg)
{
   void (*kernel)(void);

   if(status.ciaddr == 0)
   {
      message_print(PARSE_DHCP_MESSAGE);
      if(!init_dhcp(&status.ciaddr,
	            &status.siaddr,
		    &status.giaddr,
		    &status.smask))
      {
	 error_print(DHCP_TIMEOUT_ERROR);
	 return 0;
      }
   }

   do
      cebootme();
   while(tftplisten((u8 *)BL_TEMP_RAM) < 0);

   message_print(RAM_CLEAR_MESSAGE);
   memset32((u32 *)PLATFORM_MEMORY_BASE, 0, CE_CLEAR_SIZE / 4);

   message_print(LOADING_CE_MESSAGE);
   kernel = (void (*)(void))cedecode((u8 *)CE_RAM_BASE,
		                     (u8 *)BL_TEMP_RAM);

   if((u32 *)kernel != NULL)
   {
      kernel();
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// set_ip_parse
// PURPOSE: Parses a set ip command; sets the IP address of the board to either
//          the value specified, or gets one via bootp or dhcp.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_ip_parse(char const *arg)
{
   int retval = 0;

   if(*arg != 0)
   {
      u32 iaddr;

      if(cmpstr(arg, "dhcp"))
      {
	 retval =  init_dhcp(&status.ciaddr,
			     &status.siaddr,
			     &status.giaddr,
			     &status.smask);
      }
      else if(cmpstr(arg, "bootp"))
      {
	 retval =  init_bootp(&status.ciaddr,
			      &status.siaddr,
			      &status.giaddr,
			      &status.smask);
      }
      else
      {
	 if(!(iaddr = atoip(arg)))
	 {
	    error_print(PARSE_INVALID_ARG_ERROR);
	    return 0;
	 }
	 else
	 {
	    const u32 flags = FLAG_USE_STATICIP;
	    status.ciaddr = iaddr;
            (void) eeprom_write_item("FLAGS", sizeof("FLAGS")-1,
                                     &flags, sizeof(flags));
            (void) eeprom_write_item("SIP", sizeof("SIP")-1,
                                     &status.ciaddr, sizeof(status.ciaddr));
	    retval = 1;
	 }
      }
   }
   itc_printf("IP address: %s\r\n", iptoa(status.ciaddr));
   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// set_mac_parse
// PURPOSE: Parses a set mac command, and updates the MAC address of the board
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
set_mac_parse(char const *arg)
{
   u16 macaddr[3];
   char temp[5];
   int retval = 0;
   char answer;
   unsigned int num;

   if(*arg != 0)
   {
      if(itc_strlen(arg) < 12 ||
         !(atou16(arg, &macaddr[0]) &&
           atou16(arg += 4, &macaddr[1]) &&
           atou16(arg += 4, &macaddr[2])))
      {
	 error_print(PARSE_INVALID_ARG_ERROR);
	 return 0;
      }
      else
      {
         arg = next_token(arg);
	 if (*arg != '\0')
	 {
            if (!atoi (arg, &num))
	    {
	       num = 0;
	    }
	 } else
	 {
	    num = 0;
	 }

         itc_printf("\
CAUTION: Changing the MAC address can cause serious network problems.\r\n\
Are you sure you want to continue? [y/n] ");

         while (!input_byte_serial(&answer))
            idle();
         itc_printf("%c\r\n", answer);
         if (answer != 'y')
         {
            itc_printf("MAC address not changed\r\n");
            return 0;
         }

	 macaddr[0] = htons(macaddr[0]);
	 macaddr[1] = htons(macaddr[1]);
	 macaddr[2] = htons(macaddr[2]);
	 retval = write_mac_ethernet((u16 *)macaddr, (unsigned short)num);
	 init_ethernet((u16 *)status.macaddr);
      }
   }

   itc_printf("MAC address: ");
   u16toa(htons(status.macaddr[0]), temp);
   itc_printf(temp);
   u16toa(htons(status.macaddr[1]), temp);
   itc_printf(temp);
   u16toa(htons(status.macaddr[2]), temp);
   itc_printf(temp);
   itc_printf("\r\n");

   return retval;
}

////////////////////////////////////////////////////////////////////////////////
// eraseflash_parse
// PURPOSE: Parses an eraseflash command and erases flash appropriately.
// PARAMS:  (IN) char *arg - argument string
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
eraseflash_parse(char const *arg)
{
   u32 start = 0, length = 0, block=0;

   if(!get_number_parse(arg, &start))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }
   arg = next_token(arg);

   block = 128;

   if(start < 1 || start > (block - 1)) //NUM_FLASH_BLOCKS - 1
   {
      error_print(PARSE_RANGE_ERROR);
      return 0;
   }

   if(!get_number_parse(arg, &length))
   {
      char answer = 'n';

      length = block - start; //NUM_FLASH_BLOCKS - start;
      itc_printf("Do you wish to erase flash blocks %i-%i? [y/n] ",
                 start, block); //NUM_FLASH_BLOCKS-1);
      while(!input_byte_serial(&answer))
         idle();
      itc_printf("%c\r\n", answer);
      if(answer != 'y')
         return 0;
   }
   else
   {
      if((length + start) > block) //NUM_FLASH_BLOCKS)
      {
	 error_print(PARSE_RANGE_ERROR);
	 return 0;
      }
   }

   while(length-- > 0)
   {
      u32 *flash_addr = (u32 *)(PLATFORM_FLASH_BASE+(start * flash_block_size_platform()));

      itc_printf("\rErasing block %i", start++);
      if(!block_erase_flash(flash_addr))
      {
	 return 0;
      }
   }
   itc_printf("\r\n");

   return 1;
}

////////////////////////////////////////////////////////////////////////////////
// reboot_parse
// PURPOSE: Reboot the machine
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: Never returns.
////////////////////////////////////////////////////////////////////////////////
int
reboot_parse(char const *arg)
{
   void (*restart_vector)(void) = 0;

   if(!get_number_parse(arg, (u32 *)&restart_vector))
   {
       restart_vector = NULL;
   }
   itc_printf("Restarting...\r\n");
   // Wait for serial buffer to be flushed
   delay(1);
   restart_vector();
   itc_printf("Strange--we didn't restart\r\n");
   return 1;    // Should never get here
}

////////////////////////////////////////////////////////////////////////////////
// ping_parse
// PURPOSE: Sends ICMP echo request to remote node.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
ping_parse(char const *arg)
{
   u32 iaddr;

   if(!(iaddr = atoip(arg)))
   {
      error_print(PARSE_INVALID_IP_ERROR);
      return 0;
   }

   if(status.ciaddr == 0)
   {
      message_print(PARSE_DHCP_MESSAGE);
      if(!init_dhcp(&status.ciaddr,
	            &status.siaddr,
		    &status.giaddr,
		    &status.smask))
      {
	 error_print(DHCP_TIMEOUT_ERROR);
	 return 0;
      }
   }

   itc_printf("Pinging %s\r\n", iptoa(iaddr));
   icmpping(iaddr);

   // Wait a moment for the response to come in so it doesn't mess up our
   // output.
   delay(1);

   return 1;
}

//==========================================================
// pcmcia_parse
//
int
pcmcia_parse(char const *arg)
{
	if(cmpstr(arg, "insert")) {
		pcmcia(1);
	}
	else if(cmpstr(arg, "eject")) {
		pcmcia(0);
	}
	else
		return 0;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// createfis_parse
// PURPOSE: Calculates CRC on a memory range.
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
createfis_parse(char const *arg)
{
   static struct fis_image_desc fistable[NUM_FLASH_BLOCKS];
   struct fis_image_desc *fisentry = fistable;
   int blocks,totalblocks,numentries, num_block;

   // Clear out fis image to default
   memset8((u8 *)fistable, 0xff, sizeof(fistable));

   if (!get_number_parse(arg, (u32 *)&blocks) || (blocks < 1))
   {
      error_print(PARSE_INVALID_ARG_ERROR);
      return 0;
   }

   // Set up magic RedBoot entry as first block as a duplicate to user's
   // first entry.  Linux only parses the table if it sees this entry.
   itc_strcpy(fisentry->name, "RedBoot");
   fisentry->flash_base = (char *) PLATFORM_FLASH_BASE;
   fisentry->mem_base = 0;
   fisentry->size = blocks * MAIN_BLOCK_SIZE;
   fisentry->entry_point = 0;
   fisentry->data_length = blocks * MAIN_BLOCK_SIZE;
   fisentry->desc_cksum = 0;
   fisentry->file_cksum = 0;

   ++fisentry;
   numentries = 1;
   totalblocks = 0;     // First entry in loop is duplicate of RedBoot entry

   while (*arg != '\0')
   {
      if (numentries == 1)
      {
          // First entry is always for I-Boot
          itc_strcpy(fisentry->name, "I-Boot");
      } else
      {
          // Zero out name because itoa doesn't nul-terminate
          memset8((u8 *)fisentry->name, 0, sizeof(fisentry->name));
          itc_strcpy(fisentry->name, "Partition ");
          itoa(numentries, &fisentry->name[10]);
      }
      fisentry->flash_base =
         (char *) (PLATFORM_FLASH_BASE + totalblocks * MAIN_BLOCK_SIZE);
      fisentry->mem_base = 0;
      fisentry->size = blocks * MAIN_BLOCK_SIZE;
      fisentry->entry_point = 0;
      fisentry->data_length = blocks * MAIN_BLOCK_SIZE;
      fisentry->desc_cksum = 0;
      fisentry->file_cksum = 0;

      ++fisentry;
      ++numentries;
      totalblocks += blocks;

      arg = next_token(arg);
      if (*arg && (!get_number_parse(arg, (u32 *)&blocks) || (blocks < 1)))
      {
         error_print(PARSE_INVALID_ARG_ERROR);
         return 0;
      }
   }

   num_block = 128;

   if (totalblocks > (num_block - 1)) //NUM_FLASH_BLOCKS - 1))
   {
       itc_printf("Error: Too many blocks specified\r\n");
       return 0;
   }
   if (totalblocks < (num_block - 1)) //NUM_FLASH_BLOCKS - 1))
   {
       itc_printf("NOTE: %d blocks are unallocated\r\n",
                  num_block - 1 - totalblocks); //NUM_FLASH_BLOCKS - 1 - totalblocks);
   }

   itc_printf("Flashing: ");
   if (block_write_flash(
        (u32 *)(PLATFORM_FLASH_BASE + PLATFORM_FLASH_SIZE - MAIN_BLOCK_SIZE),
        (u32 *)fistable,
        numentries*sizeof(struct fis_image_desc), FLASH_DEFAULT))
   {
      return 1;
   }
   return 0;
}

#ifdef TAGGED_EEPROM
////////////////////////////////////////////////////////////////////////////////
// eeclear_parse
// PURPOSE: Clear the contents of the EEPROM, except for the MAC address
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
eeclear_parse(char const *arg)
{
    itc_printf("Clearing the contents of the EEPROM\r\n");
    nv_upgrade(1);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// eedump_parse
// PURPOSE: Dump the contents of the EEPROM
// PARAMS:  (IN) char *arg - argument string.
// RETURNS: 1 for success, 0 for failure.
////////////////////////////////////////////////////////////////////////////////
int
eedump_parse(char const *arg)
{
    eeprom_dump();
    return 0;
}
#endif // TAGGED_EEPROM
