//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      nvconfig.h
//
// Description:
//
//      Definitions for nonvolatile configuration in EEPROM.
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

#ifndef NVCONFIG_H
#define NVCONFIG_H

/* default value of 'FLAGS' item in EEPROM is ZERO. */
#define FLAG_UPDATE       1
#define FLAG_RESERVED     2
#define FLAG_USE_STATICIP 4
#define FLAG_USE_WEP      8
#define FLAG_WEP_128BIT   16
#define FLAG_WEP_HEX      32
#define FLAG_MANUAL_PROXY 64
#define FLAG_TABLE_BUSY   128
#define FLAG_USE_PROXY    256

// SYSTEM
void nv_setup(void);
void nv_upgrade(int force);

#endif // NVCONFIG_H
