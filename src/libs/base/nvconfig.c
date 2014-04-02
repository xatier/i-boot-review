//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      nvconfig.c
//
// Description:
//
//      Handle nonvolatile configuration in EEPROM.
//
// Author:
//
//      Dan Fandrich
//
// Created:
//
//      September 2002
//
////////////////////////////////////////////////////////////////////////////////

#include <debug.h>
#include <eeprom.h>
#include <ethernet.h>
#include <parser.h>
#include <c_main.h>
#include <cpu.h>
#include <nvconfig.h>

////////////////////////////////////////////////////////////////////////////////
// nv_upgrade
// PURPOSE: Upgrade the contents of the EEPROM into tagged format
// PARAMS:  (IN) int force - Force EEPROM to be cleared if nonzero, otherwise
//                           only if it is not already in tagged format
// RETURNS: None.
// NOTES:   The MAC address(es) are preserved, if possible.
////////////////////////////////////////////////////////////////////////////////
void
nv_upgrade(int force)
{
    u16 len = 0;

    // If the EEPROM isn't in tagged format, or a CRC error is discovered
    // while searching for a nonexistent tag, clear the EEPROM.
    if (force || !check_eeprom_header() ||
        (eeprom_find_item("__check__", sizeof("__check__")-1, &len) !=
         EEPROM_EOF))
    {
        u16 mac0[3];
        u16 mac1[3];
        const u16 mac_empty = 0xffff;
        int have_first_mac, have_second_mac;

        itc_printf("Storing EEPROM contents in tagged format\r\n");

        // Read both MAC addresses before writing anything into EEPROM
        have_first_mac = read_mac_ethernet(mac0, 0);
        have_second_mac = read_mac_ethernet(mac1, 1);

        // Write the tagged header and EOF tag
        clear_eeprom();

        // The MAC address must appear first in the EEPROM in order to
        // maintain backward compatibility with older versions of I-Boot
        // that expect the MAC address at a fixed location.  Besides, the
        // MAC address is a part of the device and should always exist.
        if (have_first_mac &&
            !(mac0[0] == mac_empty &&
              mac0[1] == mac_empty &&
              mac0[2] == mac_empty))
        {
            if (eeprom_write_item("MACAD0", sizeof("MACAD0")-1,
                (u8 *)mac0, sizeof(mac0)) < 0)
            {
                itc_printf("Could not write to EEPROM\r\n");
            }
        } else
        {
            // This could happen if the EEPROM was corrupted
            itc_printf("Warning: No MAC address was found; set it now\r\n");
        }

        // If we read a second MAC address without error, and that address
        // appears to be non-blank, write a tag for it.
        if (have_second_mac &&
            !(mac1[0] == mac_empty &&
              mac1[1] == mac_empty &&
              mac1[2] == mac_empty))
        {
            if (eeprom_write_item("MACAD1", sizeof("MACAD1")-1,
                (u8 *)mac1, sizeof(mac1)) < 0)
            {
                itc_printf("Could not write to EEPROM\r\n");
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// nv_setup
// PURPOSE: Read the EEPROM to get the configuration settings.
// PARAMS:  None.
// RETURNS: None.
////////////////////////////////////////////////////////////////////////////////
void
nv_setup(void)
{
    u32 speed = 0, flags = 0;// zero these in case the EEPROM only holds 16 bits
    u16 size;

    // Initialize the EEPROM into tagged format, if it isn't already
    nv_upgrade(0);

    size = sizeof(speed);
    if (eeprom_get_item("CPUCLK", sizeof("CPUCLK")-1, &speed, &size) > 0)
    {
        int newspeed;
        itc_printf("CPU speed set to ");
        newspeed = set_cpu_speed(speed);
        itc_printf("%d MHz\r\n", newspeed);
    }

    // Only read the static IP settings if the flags bit indicates to use them
    size = sizeof(flags);
    if ((eeprom_get_item("FLAGS", sizeof("FLAGS")-1, &flags, &size) > 0) &&
        (flags & FLAG_USE_STATICIP))
    {
        size = sizeof(status.ciaddr);
        if (eeprom_get_item("SIP", sizeof("SIP")-1, &status.ciaddr, &size) > 0)
        {
            itc_printf("IP address: %s\r\n", iptoa(status.ciaddr));
        }

        size = sizeof(status.smask);
        if (eeprom_get_item("SMASK", sizeof("SMASK")-1, &status.smask, &size) > 0)
        {
            itc_printf("Subnet Mask: %s\r\n", iptoa(status.smask));
        }

        size = sizeof(status.giaddr);
        if (eeprom_get_item("SGATE", sizeof("SGATE")-1, &status.giaddr, &size) > 0)
        {
            itc_printf("Gateway IP address: %s\r\n", iptoa(status.giaddr));
        }

    }
}
