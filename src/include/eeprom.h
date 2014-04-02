/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: eeprom.h
 *
 * FUNCTION:
 * 	This is the EEPROM Header.  Include this if you need access to EEPROMS
 ****************************************************************************/

#ifndef __EEPROM_H__
#define __EEPROM_H__

#include <types.h>

typedef enum {
    EEPROM_CLEAR_OK     = 1,
    EEPROM_EOF          = 0,

    EEPROM_ERR_READ     = -1,
    EEPROM_ERR_SECTION  = -2,
    EEPROM_ERR_CRC      = -3,
    EEPROM_ERR_WRITE    = -4,
} eeprom_status;

#define EEPROM_HEADER_SIZE 8    // length of header section

#ifdef TAGGED_EEPROM

int init_eeprom_tagged(void);
int check_eeprom_header(void);
int clear_eeprom(void);
int eeprom_dump(void);

////////////////////////////////////////////////////////////////////////////////
// eeprom_write_item
// PURPOSE: Write a Key and associated Data to the EEPROM.
// PARAMS:  (IN) void* key   - Key to lookup in eeprom.
//          (IN) u16 keylen  - Length of the key.
//          (OUT)void* data  - Data associated with the key.
//          (OUT)u16 datalen - Length of the data returned.
// RETURNS:
//          >0 = Offset
//           0 = Not Found (EOF)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
//          -3 = INVALID_CRC
//          -4 = WRITE_ERROR
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int eeprom_write_item(void const *key, u16 keylen, void const *data, u16 datalen);

////////////////////////////////////////////////////////////////////////////////
// eeprom_get_item
// PURPOSE: Read Data associated with a Key from EEPROM.
// PARAMS:  (IN) void* key  - Key to lookup in eeprom.
//          (IN) u16 keylen - Length of the key.
//          (OUT)void* data - Data associated with the key.
//                            NULL will not store the data.
//          (IN/OUT)u16* len - Length of buffer/number of data bytes returned.
// RETURNS:
//          >0 = Offset
//           0 = Not Found (EOF)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
//          -3 = INVALID_CRC
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int eeprom_get_item(const void* key, u16 keylen, void* data, u16* len);

////////////////////////////////////////////////////////////////////////////////
// eeprom_find_item
// PURPOSE: Read Data associated with a Key from EEPROM.
// PARAMS:  (IN) void* key       - Key to lookup in eeprom.
//          (IN) u16 keylen      - Length of the key.
//          (OUT)u16* datalength - Length of the associated data.
// RETURNS:
//          >0 = Offset
//           0 = Not Found (EOF)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
//          -3 = INVALID_CRC
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int eeprom_find_item(const void* key, u16 keylen, u16* datalength);


////////////////////////////////////////////////////////////////////////////////
// eeprom_delete_item
// PURPOSE: Delete a Key and associated Data to the EEPROM.
// PARAMS:  (IN) void* key   - Key to lookup in eeprom.
//          (IN) u16 keylen  - Length of the key.
// RETURNS:
//          >0 = Offset
//           0 = Not Found (EOF)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
//          -3 = INVALID_CRC
//          -4 = WRITE_ERROR
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int
eeprom_delete_item(const void* key, u16 keylen);

#else

// Stub this out to more easily make settings nonvolatile
#define eeprom_write_item(k,kl,d,dl) (EEPROM_ERR_WRITE)

#endif // TAGGED_EEPROM

 
#endif // __EEPROM_H__
