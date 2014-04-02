#ifndef _FIS_H_
#define _FIS_H_

struct fis_image_desc {
    unsigned char name[16];      // Null terminated name
    char *        flash_base;    // Address within FLASH of image
    char *        mem_base;      // Address in memory where it executes
    unsigned long size;          // Length of image
    char *        entry_point;   // Execution entry point
    unsigned long data_length;   // Length of actual data
    unsigned char _pad[256-(16+4*sizeof(unsigned long)+3*sizeof(char *))];
    unsigned long desc_cksum;    // Checksum over image descriptor
    unsigned long file_cksum;    // Checksum over image data
};

#endif // _FIS_H_
