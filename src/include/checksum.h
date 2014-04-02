////////////////////////////////////////////////////////////////////////////////
// Module name:
//
//      checksum.h
//
// Description:
//      Functions for generating 32 bit CRCs.
////////////////////////////////////////////////////////////////////////////////

#ifndef CHECKSUM_H
#define CHECKSUM_H

#define INITIAL_CRC32 0

void init_crc_table(void);
unsigned long update_crc(unsigned long crc32val,
                         unsigned char const *data_blk_ptr,
                         int len);

#endif //CHECKSUM_H
