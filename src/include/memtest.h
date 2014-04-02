#ifndef _MEMTEST_H_
#define _MEMTEST_H_

#define MAX_UINT 0xFFFFFFFF
#define INTERVAL 0x100000

#define WRITE_REG(reg,val) (*(volatile unsigned int *)(reg) = (val))
#define READ_REG(reg)      (*(volatile unsigned int *)(reg))

// function declarations
int do_walking_ones (unsigned int start_address, unsigned int stop_address);
int do_walking_zeros (unsigned int start_address, unsigned int stop_address);
int do_streaming_ones (unsigned int start_address, unsigned int stop_address);
int do_streaming_zeros (unsigned int start_address, unsigned int stop_address);
int do_number_test (unsigned int start_address, unsigned int stop_address);

#endif // _MEMTEST_H_
