#ifndef CRC16_H
#define CRC16_H

#include <types.h>

#define CRC_TABLE 1

#define INITIAL_CRC 0

u16 getcrc16(u8 const * bufptr, int Length, u16 crc);

#endif // CRC16
