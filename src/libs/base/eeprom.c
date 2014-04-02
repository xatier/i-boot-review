/*****************************************************************************
 * Copyright (c) 2002, Intrinsyc Software Inc.
 *
 * FILE: eeprom.c
 *
 * FUNCTION:
 * 	This is the EEPROM storage driver for the Bootloader.
 *
 ****************************************************************************/

//We want failures to get to the user
#define _DEBUG
#define _DEBUG_FAIL

#include <types.h>
#include <debug.h>
#include <string.h>
#include <eeprom_24lc64.h>
#include <eeprom.h>
#include <util.h>
#include <crc16.h>

#define EEPROM_HEADER_MAGIC_0 0xEF
#define EEPROM_HEADER_MAGIC_1 0xBE

#define EXTENDED_LENGTH_FLAG 0x80

typedef struct
{
    u16 length;
    u8 * data;
} eeField;

typedef struct
{
    eeField tag;
    eeField value;
} eeItem;

static int write_eof(u16 offset);

////////////////////////////////////////////////////////////////////////////////
// init_eeprom_tagged
// PURPOSE: Initialize the EEPROM into tagged format if it isn't already.
// PARAMS:  None.
// RETURNS: 0 on error, 1 on success
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int
init_eeprom_tagged(void)
{
    if (!check_eeprom_header())
    {
        DEBUG("Warning: EEPROM not initialized!\r\n");\
        return 0;
    }
            
    return 1;
} 

////////////////////////////////////////////////////////////////////////////////
// clear_eeprom
// PURPOSE: Write an EEPROM tagged header and EOF record to the EEPROM
// PARAMS:  None.
// RETURNS: 0 on error, 1 on success
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int
clear_eeprom(void)
{
    u8 header[EEPROM_HEADER_SIZE];
    u16 len;
    
    memset8(header, 0, sizeof(header));
    header[0] = EEPROM_HEADER_MAGIC_0;
    header[1] = EEPROM_HEADER_MAGIC_1;
    len = EEPROM_HEADER_SIZE;
    if (!write_data_eeprom (0, &header[0], &len) ||
        !write_eof(EEPROM_HEADER_SIZE))
    {
        DEBUG_FAIL("Error initializing EEPROM\r\n");
        return 0;
    }
            
    return 1;
} 

////////////////////////////////////////////////////////////////////////////////
// check_eeprom_header
// PURPOSE: Check the EEPROM header to verify that it is in tagged format
// PARAMS:  None.
// RETURNS: 0 on error, 1 on success
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int
check_eeprom_header(void)
{
    u8 head[EEPROM_HEADER_SIZE];
    u16 len = EEPROM_HEADER_SIZE;
    
    if (!read_data_eeprom(0, &head[0], &len))
    {
        return 0;
    }
    if ((head[0] != EEPROM_HEADER_MAGIC_0) ||
        (head[1] != EEPROM_HEADER_MAGIC_1))
    {
        return 0;
    }
            
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// create_item
// PURPOSE: Create an eeItem by storing pointers into the structure
// PARAMS:  (OUT)eeField* item - Structured view of the Raw data.
//          (IN) void* key  - Key to lookup in EEPROM.
//          (IN) u16 keylen - Length of the key.
//          (OUT)void* data - Data associated with the key.
//          (OUT)u16* datalen - Length of the data returned.
// RETURNS: None.
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
inline static void
create_item(eeItem* item, u8 const *key, u16 keylen, u8 const *data, u16 datalen)
{
    item->tag.length = keylen;
    item->tag.data   = (char *) key;
    item->value.length = datalen;
    item->value.data   = (char *) data;
}

////////////////////////////////////////////////////////////////////////////////
// compress_length
// PURPOSE: Write a compressed length into the buffer
// PARAMS:  (OUT) u8 *buf - buffer to write into
//          (IN) int length - length value
// RETURNS: Number of bytes taken up by compressed length (maximum of 2)
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
inline static int
compress_length(u8 *buf, int length)
{
    if (length < 128)
    {
        // Length compression
        buf[0] = length;
        return 1;
    }
    else
    {
        buf[0] = (length >> 8) | EXTENDED_LENGTH_FLAG;
        buf[1] = length & 0xff;
        return 2;
    }
}

////////////////////////////////////////////////////////////////////////////////
// record_length
// PURPOSE: Calculate the amount of space required to store this record
// PARAMS:  (OUT) eeItem *item - record
// RETURNS: Number of bytes taken up by record in EEPROM
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
inline static int
record_length(eeItem const *item)
{
    u8 buf[2];
    int len = compress_length(buf, item->tag.length);
    len += item->tag.length;
    len += compress_length(buf, item->value.length);
    len += item->value.length;
    len += 2; // CRC
    return len;
}

////////////////////////////////////////////////////////////////////////////////
// write_record
// PURPOSE: Write a tagged record to the EEPROM
// PARAMS:  (IN) u16 offset - Start of item in EEPROM.
//          (OUT)eeItem *item - Tagged item
// RETURNS: 0 on error, 1 on success
// NOTES:   item->rawlength is set by this function.
////////////////////////////////////////////////////////////////////////////////
static int
write_record(u16 offset, eeItem const * item)
{
    u16 keylen_space, datalen_space, bytes, crc;
    u8 buf[2];

    // Write the tag record -- length
    bytes = keylen_space = compress_length(buf, item->tag.length);
    crc = getcrc16(buf, bytes, INITIAL_CRC); 
    if (!write_data_eeprom(offset, buf, &bytes))
        return 0;
    offset += keylen_space;

    // Write the tag record -- value
    bytes = item->tag.length;
    crc = getcrc16(item->tag.data, bytes, crc); 
    if (!write_data_eeprom(offset, item->tag.data, &bytes))
        return 0;
    offset += item->tag.length;

    // Write the data record -- length
    bytes = datalen_space = compress_length(buf, item->value.length);
    crc = getcrc16(buf, bytes, crc); 
    if (!write_data_eeprom(offset, buf, &bytes))
        return 0;
    offset += datalen_space;

    // Write the data record -- value
    bytes = item->value.length;
    crc = getcrc16(item->value.data, bytes, crc); 
    if (!write_data_eeprom(offset, item->value.data, &bytes))
        return 0;
    offset += item->value.length;

    // Write the CRC
    // Note that this is written in big-endian format for some reason
    buf[0] = (crc >> 8) & 0xff;
    buf[1] = crc & 0xff;
    bytes = 2;

    return write_data_eeprom(offset, buf, &bytes);
}

////////////////////////////////////////////////////////////////////////////////
// write_eof
// PURPOSE: Write an EOF record to the EEPROM
// PARAMS:  (IN) u16 offset - Start of item in EEPROM.
// RETURNS: CRC value
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
static int
write_eof(u16 offset)
{
    eeItem eof;
    eof.tag.length = 0;
    eof.tag.data = 0;
    eof.value.length = 0;
    eof.value.data = 0;
    return write_record(offset, &eof);
}

////////////////////////////////////////////////////////////////////////////////
// read_crc
// PURPOSE: Read the record CRC from the EEPROM
// PARAMS:  (IN) u16 offset - Start of item in EEPROM.
//          (OUT)u8* dest - Destination of the raw CRC bytes (2 bytes).
// RETURNS: CRC value
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
inline static u16
read_crc(u16 offset, u8* dest)
{
    u16 bytes_read = 2;

    if (read_data_eeprom(offset, dest, &bytes_read))
    {
        return ((dest[0] << 8) | dest[1]); 
    }
    return 0xffff;
}

        
////////////////////////////////////////////////////////////////////////////////
// read_section
// PURPOSE: Read a Key or Data Item in its raw form.
// PARAMS:  (IN) u16 next - Start of section in EEPROM.
//          (OUT)u8* dest - Destination of the raw data.
//          (OUT)eeField* field - Structured view of the Raw data.
// RETURNS:
//          >0 = Raw record length (Actual bytes read)
//           0 = Raw record length (Actual bytes read)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
static int
read_section(u16 next, u8* dest, eeField* field)
{
    u8* tmp = dest;
    const u16 begin = next;
    u16 bytes_read = 1;

    // Check for read beyond end of device.
    if (begin >= MAX_EEPROM_SIZE)
        return EEPROM_ERR_READ;

    if (read_data_eeprom(next++, tmp, &bytes_read))
    {
        if (*tmp & EXTENDED_LENGTH_FLAG)
        {
            field->length = ((*tmp & 0x7f) << 8);
            if (!read_data_eeprom(next++, tmp+1, &bytes_read))
                return EEPROM_ERR_READ;
            field->length |= (*(tmp+1) & 0xff);
        }
        else
        {
            field->length = (*tmp);
        }
        // Check for reasonable length.
        if (field->length > (MAX_EEPROM_SIZE - begin))
            return EEPROM_ERR_SECTION;

        bytes_read = field->length;
        field->data = NULL;
        if (bytes_read > 0)
        {
            if (read_data_eeprom(next, (tmp+(next-begin)), &bytes_read))
            {
                if (bytes_read != field->length)
                    return EEPROM_ERR_READ;

                field->data = tmp+(next-begin);
                next += bytes_read; 
            }
            else
                return EEPROM_ERR_READ;
        }
    }
    else
        return EEPROM_ERR_READ;
    
   return (next - begin); // OK
}

////////////////////////////////////////////////////////////////////////////////
// read_record
// PURPOSE: Read a Key and Data pair in its raw form.
// PARAMS:  (IN) u16 offset - Start of item in EEPROM.
//          (OUT)u8* dest   - Destination of the raw data.
//          (OUT)eeItem* item - Structured view of the Raw data.
// RETURNS:
//          >0 = Raw record length (Actual bytes read)
//           0 = Not Found (EOF)
//          -1 = READ_ERROR
//          -2 = INVALID_SECTION
//          -3 = INVALID_CRC
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
static int
read_record(u16 offset, u8* dest, eeItem* item)
{
    int bytes;
    u16 crc;
    const u16 begin = offset;

    if (!check_eeprom_header())
    {
        DEBUG_FAIL("Error reading EEPROM header\r\n");
        return EEPROM_ERR_READ;
    }

    bytes = read_section(offset, dest, &item->tag);
    if (bytes < 0)
        return bytes;

    offset += bytes;
    bytes = read_section(offset, dest+bytes, &item->value);
    if (bytes < 0)
        return bytes;

    offset += bytes;
    crc = read_crc(offset, dest+(offset-begin));

    if (crc != getcrc16(dest, offset-begin, INITIAL_CRC))
    {
        return EEPROM_ERR_CRC;
    }
    offset += 2; // Skip CRC

    if ((item->tag.length == 0) && (item->value.length == 0))
    {
        return EEPROM_EOF;
    }

    return offset-begin;
}

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
int
eeprom_get_item(const void* key, u16 keylen, void* data, u16* len)
{
    int retval = EEPROM_EOF;
    u16 next = EEPROM_HEADER_SIZE;
    int bytes = 0;
    u8 tmp[MAX_EEPROM_SIZE];
  
    eeItem item;

    if (!check_eeprom_header())
    {
        DEBUG_FAIL("Error reading EEPROM header\r\n");
        return EEPROM_ERR_READ;
    }

    do 
    {
        next += bytes;
        bytes = read_record(next, tmp, &item);
        if (bytes <= 0)
        {
            retval = bytes;
            break;
        }
        retval = next;
    } while (item.tag.length != keylen || itc_memcmp(key, item.tag.data, keylen));

    if (retval > 0)
    {
        if (data)
            itc_memcpy(data, item.value.data, MIN(item.value.length, *len));
        *len = item.value.length;
        retval = next;
    }

    return retval;
}

////////////////////////////////////////////////////////////////////////////////
// eeprom_dump
// PURPOSE: Debug function to display the raw tags contained in the EEPROM.
// PARAMS:  None.
// RETURNS:
//          >0 = Offset
//          -1 = READ_ERROR
// NOTES:   None.
////////////////////////////////////////////////////////////////////////////////
int eeprom_dump(void)
{
    int retval = 0;
    u16 next = EEPROM_HEADER_SIZE;
    int bytes = 0;
  
    eeItem item;

    u8 tmp[MAX_EEPROM_SIZE];

    if (!check_eeprom_header())
    {
        DEBUG_FAIL("Error reading EEPROM header\r\n");
        return EEPROM_ERR_READ;
    }

    do 
    {
        next += bytes;
        bytes = read_record(next, tmp, &item);
        if (bytes <= 0)
        {
            retval = bytes;
            break;
        }
        retval = next;

        item.tag.data[item.tag.length] = 0;
        itc_printf("%s\r\n",item.tag.data); 
        print_bytes(item.value.data, item.value.length);

    } while (1);

    if (retval < 0)
    {
        DEBUG_FAIL("Error %d reading EEPROM\r\n", retval);
    }
    return retval;
}


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
int
eeprom_find_item(const void* key, u16 keylen, u16* datalength)
{
    return eeprom_get_item(key, keylen, NULL, datalength);
}

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
int
eeprom_write_item(void const *key, u16 keylen, void const *data, u16 datalen)
{
    int retval = EEPROM_EOF;
    u16 next = EEPROM_HEADER_SIZE;
    int bytes = 0;
    u16 space_needed;
    eeItem item;
    eeItem changed_item;

    u8 tmp[MAX_EEPROM_SIZE];

    if (!check_eeprom_header())
    {
        DEBUG_FAIL("Error reading EEPROM header\r\n");
        return EEPROM_ERR_READ;
    }

    // Find the Item in the list.
    do 
    {
        next += bytes;
        bytes = read_record(next, tmp, &item);
        if (bytes <= 0)
        {
            retval = bytes;
            break;
        }
        retval = next;
    } while (item.tag.length != keylen || itc_memcmp(key, item.tag.data, keylen));

    // Save the new item.
    create_item(&changed_item, key, keylen, data, datalen);
    space_needed = record_length(&changed_item);
       
    // We found the Item or EOF
    if (retval > 0) 
    {
        // We found the Item, try to update
        if (space_needed == bytes)
        {
            // Same size, so replace in place.
            write_record(next, &changed_item);
            retval = next;
        }
        else
        {
            // Delete Item and append to the end of the list.
            u16 read_ptr = next;
            u16 write_ptr = next;
            u16 byte_count;

            do
            {
                read_ptr += bytes; // Skip forward.
                bytes = read_record(read_ptr, tmp, &item);
                if (bytes <= 0)
                {
                    retval = bytes;
                    break;
                }
                byte_count = bytes;
                if (!write_data_eeprom(write_ptr, tmp, &byte_count))
                {
                    retval = EEPROM_ERR_WRITE;
                    break;
                }
                write_ptr += bytes;
            } while (1);

            if (retval >= 0)
            {
                retval = write_ptr;
                if (write_record(write_ptr, &changed_item))
                {
                    write_ptr += space_needed;
                    if (!write_eof(write_ptr))
                        retval = EEPROM_ERR_WRITE;
                }
                else
                    retval = EEPROM_ERR_WRITE;
            }
        }
    }
    else if (retval == EEPROM_EOF)
    {
        // EOF, just append item and new EOF
        retval = next;
        if (write_record(next, &changed_item))
        {
            next += space_needed;
            if (!write_eof(next))
                retval = EEPROM_ERR_WRITE;
        }
    }

    return retval;
}

#ifdef NEED_EEPROM_DELETE
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
eeprom_delete_item(const void* key, u16 keylen)
{
    int retval = EEPROM_EOF;
    u16 next = EEPROM_HEADER_SIZE;
    int bytes = 0;
   
    eeItem item;

    u8 tmp[MAX_EEPROM_SIZE];

    if (!check_eeprom_header())
    {
        DEBUG_FAIL("Error reading EEPROM header\r\n");
        return EEPROM_ERR_READ;
    }

    // Find the Item in the list.
    do 
    {
        next += bytes;
        bytes = read_record(next, tmp, &item);
        if (bytes <= 0)
        {
            retval = bytes;
            break;
        }
        retval = next;
    } while (item.tag.length != keylen || itc_memcmp(key, item.tag.data, keylen));

    // We found the Item or EOF
    if (retval > 0) 
    {
        // We found the Item, try to update
        // Delete Item and append to the end of the list.
        u16 read_ptr = next;
        u16 write_ptr = next;
        u16 byte_count;

        do
        {
            read_ptr += bytes; // Skip forward.
            bytes = read_record(read_ptr, tmp, &item);
            if (bytes <= 0)
            {
                retval = bytes;
                break;
            }
            byte_count = bytes;
            if (!write_data_eeprom(write_ptr, tmp, &byte_count))
            {
                retval = EEPROM_ERR_WRITE;
                break;
            }
            write_ptr += bytes;
        } while (1);

        if (retval >= 0)
        {
            retval = write_ptr;
            if (!write_eof(write_ptr))
                retval = EEPROM_ERR_WRITE;
        }
    }

    return retval;
}
#endif // NEED_EEPROM_DELETE

