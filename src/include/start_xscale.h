#ifndef START_H
#define START_H

#include <config.h>

#define SDRAM_BASE        0xA0000000

//Interrupt Control Registers
#define INTERRUPT_CONTROL_BASE 0x40D00000

#define ICIP              0x00
#define ICMR              0x04
#define ICLR              0x08
#define ICFP              0x0C
#define ICPR              0x10
#define ICCR              0x14

#define CLOCK_MANAGER_BASE 0x41300000

#define CCCR              0x00
#define CKEN              0x04
#define OSCC              0x08

#define CCCR_VALUE        0x00000241

#define CKEN_VALUE        0x00000040
#define OSCC_VALUE        0x00000002

#define OSCR              0x40A00010
#define CLK_TO_10MS	      36864         // 3.686400 Mhz

//GPIO registers
#define GPIO_BASE         0x40E00000

#define GPLR0             0x00
#define GPLR1             0x04
#define GPLR2             0x08
#define GPDR0             0x0C
#define GPDR1             0x10
#define GPDR2             0x14
#define GPSR0             0x18
#define GPSR1             0x1C
#define GPSR2             0x20
#define GPCR0             0x24
#define GPCR1             0x28
#define GPCR2             0x2C
#define GRER0             0x30
#define GRER1             0x34
#define GRER2             0x38
#define GFER0             0x3C
#define GFER1             0x40
#define GFER2             0x44
#define GDER0             0x48
#define GDER1             0x4C
#define GDER2             0x50
#define GAFR0_L           0x54
#define GAFR0_U           0x58
#define GAFR1_L           0x5C
#define GAFR1_U           0x60
#define GAFR2_L           0x64
#define GAFR2_U           0x68

//GPIO initial values
#define GPDR0_VALUE       0xC3C0A940
#define GPDR1_VALUE       0xFCFFAB82
#define GPDR2_VALUE       0x0001FFFF

#define GPSR0_VALUE       0x00008000
#define GPSR1_VALUE       0x00BFA882
#define GPSR2_VALUE       0x0001C000

#define GPCR0_VALUE       0xC3C02940
#define GPCR1_VALUE       0xFC400300
#define GPCR2_VALUE       0x00003FFF

#define GRER0_VALUE       0x00000000
#define GRER1_VALUE       0x00000000
#define GRER2_VALUE       0x00000000

#define GFER0_VALUE       0x00000000
#define GFER1_VALUE       0x00000000
#define GFER2_VALUE       0x00000000

#define GAFR0L_VALUE      0x98411004
#define GAFR0U_VALUE      0xA61A8150
#define GAFR1L_VALUE      0x699A9558
#define GAFR1U_VALUE      0xAAA5AAAA
#define GAFR2L_VALUE      0xAAAAAAAA
#define GAFR2U_VALUE      0x00000002

//Memory Control Registers
#define MEM_CTL_BASE      0x48000000

#define MDCNFG            0x00
#define MDREFR            0x04
#define MSC0              0x08
#define MSC1              0x0C
#define MSC2              0x10
#define MECR              0x14
#define SXCNFG            0x1C
#define MCMEM0            0x28
#define MCMEM1            0x2C
#define MCATT0            0x30
#define MCATT1            0x34
#define MCIO0             0x38
#define MCIO1             0x3C
#define MDMRS             0x40

#define MDREFR_SLFRSH   0x00400000
#define MDREFR_E0PIN    0x00001000
#define MDREFR_E1PIN    0x00008000
#define MDCNFG_DE0      0x00000001
#define MDCNFG_DE1      0x00000002
#define MDCNFG_DE2      0x00010000
#define MDCNFG_DE3      0x00020000

//Memory Control Register initial values
#define MDCNFG64_VALUE      0x00001AC9
#define MDREFR_VALUE	0x000FC018	//0x000DC018

//#ifdef CS8900A_ETHERNET
// Timings for Crystal CS8900a Ethernet chip and flash memory
//#define MSC0_VALUE        0x3FF83FD0    // should really be ....23D3
//#else
// Timings for SMSC 91C111 Ethernet chip and flash memory
#define MSC0_VALUE        0x23F27FF8//0x23F223FA	// should really be ....23D3
//#endif

#define MSC1_VALUE        0x3FF1A441
#define MSC2_VALUE        0x7FF47FF1
#define MECR_VALUE        0x00000000
#define SXCNFG_VALUE      0x00000000
#define MCMEM0_VALUE      0x00010504
#define MCMEM1_VALUE      0x00010504
#define MCATT0_VALUE      0x00010504
#define MCATT1_VALUE      0x00010504
#define MCIO0_VALUE       0x00004715
#define MCIO1_VALUE       0x00004715
#define MDMRS_VALUE       0x00000000

#define FLYCNFG_VALUE     0x01FE01FE

// Power management
#define RCSR              0x40F00030
#define RCSR_SLEEP        0x00000004
#define PSPR              0x40F00008
#define PSSR              0x40F00004
#define PSSR_PH           0x00000010
#define PSSR_RDH          0x00000020
#define PSSR_STATUS_MASK  0x00000007

//FFUART Registers
#define FFUART_BASE       0x40100000

#define FFRBR             0x00
#define FFTHR             0x00
#define FFIER             0x04
#define FFIIR             0x08
#define FFFCR             0x08
#define FFLCR             0x0C
#define FFMCR             0x10
#define FFLSR             0x14
#define FFMSR             0x18
#define FFSPR             0x1C
#define FFISR             0x20
#define FFDLL             0x00
#define FFDLH             0x04

//FFUART Register initial values (9600 bps, 8N1)
#define FFIER_VALUE       0x00000040
#define FFFCR_VALUE       0x00000041
#define FFLCR_VALUE_SETUP 0x00000083
#define FFLCR_VALUE_USAGE 0x00000003    // 0x03 is 8 bits, 1 stop, no parity
#define FFMCR_VALUE       0x00000000
#define FFSPR_VALUE       0x00000000
#define FFISR_VALUE       0x00000000
#define FFDLL_VALUE       0x00000008    // 0x08 \ is 115200 bps
#define FFDLH_VALUE       0x00000000    // 0x00 /

#endif //START_H
