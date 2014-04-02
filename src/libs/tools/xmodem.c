#include <serial.h>
#include <timer.h>
#include <util.h>
#include <crc16.h>
#include <xmodem.h>

extern u8 *download_buffer;
extern long download_length;

extern char gInputBuf[];

#define Sleep(x) delay((x)/1000)

#define TIMEOUT_UPLOAD   90
#define TIMEOUT_DOWNLOAD 90
#define MAX_RESEND_COUNT 10     // Max number of times to resend packet
#define MAX_START_COUNT  20

#define MAX_BLOCK_LEN    1024

#define SOH              ((unsigned char) 0x01) // start of 128 byte block
#define STX              ((unsigned char) 0x02) // start of 1024 byte block
#define NAK              ((unsigned char) 0x15)
#define ACK              ((unsigned char) 0x06)
#define EOT              ((unsigned char) 0x04)
#define CAN              ((unsigned char) 0x18)
#define XCRC             ((unsigned char) 0x43) // NAK with CRC
#define CESC             ((unsigned char) 0x1b)



#define StoreDirect(typ,offset,val) \
{                                   \
   typ theval = val;               \
   itc_memcpy(offset,&theval,sizeof(typ));\
}


int
Xrecv(u8 *pBuffer, u32 *piLength)
{
    u16 testcrc;
    int ercode;
    int startcount = MAX_START_COUNT;
    u8  cdata;
    u8  locrc, hicrc;
    u8  mylocrc, myhicrc;
    int blocklen;
    u8 lastblock = 0;
    static u8 mydata[MAX_BLOCK_LEN+2];

    *piLength = 0;

    Sleep(3000);
    do
    {
        output_byte_serial(XCRC);
        // Wait for response from our initial NAK
        ercode = raw_input_serial(&cdata, 1, 1000);
        if (ercode && ((cdata == SOH) || (cdata == STX) || 
                       (cdata == CESC) || (cdata == CAN)))
        {
            break;
        }

        // wait for 1/2 second of silence in case we're getting lots of garbage
        while (raw_input_serial(&cdata, 1, 50))
            ;
    } while (--startcount > 0);

    // Timed out
    if (startcount == 0)
    {
        Sleep(5000);
        return 1;
    }


    if (cdata == CAN || cdata == CESC)  // somehow canceled
    {
        Sleep(1000);
        return 2;
    }
    do
    {
        int blockfailed = 0;
        u8 block;
        u8 iblock;

        blocklen = (cdata == STX) ? 1024 : 128;

        // Get block number
        if (raw_input_serial(&block, 1, 100) == 0)
        {
            blockfailed = 1;
        }

        // Get inverted block number
        if (raw_input_serial(&iblock, 1, 100) == 0)
        {
            blockfailed = 1;
        }

        // Check that block numbers are consistent
        blockfailed |= (block != (u8) ~iblock);

        // Read data block
        ercode = raw_input_serial(mydata, blocklen+2, 500);  // 128 data + crchi crclo

        blockfailed |= (ercode == 0);

        testcrc = getcrc16(mydata, blocklen, INITIAL_CRC);
        myhicrc = (testcrc >> 8) & 0xff;
        mylocrc = testcrc & 0xff;

        hicrc = mydata[blocklen+0];
        locrc = mydata[blocklen+1];

        // Check that CRCs match
        blockfailed |= (locrc != mylocrc || hicrc != myhicrc);

        // Check that we're getting the next block, or the last one repeated
        if (block != (u8)(lastblock+1) && block == lastblock)
        {
            // What happened???
            blockfailed = 1;
        }

        if (blockfailed)
        {
            // wait for 1/2 second of silence to get rid of any more buffered
            // blocks coming that are way out of date.
            while (raw_input_serial(&cdata, 1, 50))
                ;
            output_byte_serial(XCRC);
        }
        else
        {
            output_byte_serial(ACK);
            // Don't copy this block's data if it's a duplicate block
            if (block != lastblock)
            {
                itc_memcpy(pBuffer, mydata, blocklen);
                *piLength += blocklen;
                pBuffer += blocklen;
                lastblock = block;
            }
        }

        // Wait 10 seconds for beginning of next block & send NAKs until
        // we get it
        startcount = MAX_START_COUNT;
        do {
            ercode = raw_input_serial(&cdata, 1, 1000);
            if ((ercode != 0) && ((cdata == EOT) ||
                                  (cdata == SOH) || (cdata == STX) ||
                                  (cdata == CESC) || (cdata == CAN)))
                break;
            output_byte_serial(XCRC);
        } while (startcount-- > 0);

        if (startcount == 0)
        {
            Sleep(5000);
            return 1;
        }

        if (cdata == EOT)
        {
            output_byte_serial(ACK);
            Sleep(1000);
            return 0;           // 0 for successful transfer
        }

        if (cdata == CAN || cdata == CESC)  // somehow canceled
        {
            Sleep(5000);
            return 2;
        }
    } while (1);
}
