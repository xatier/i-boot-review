#ifndef START_SA_H
#define START_SA_H

#define GPIO_BASE               0x90040000
#define GPLR                    0x00
#define GPDR                    0x04
#define GPSR                    0x08
#define GPCR                    0x0C
#define GRER                    0x10
#define GFER                    0x14
#define GEDR                    0x18
#define GAFR                    0x1C

#ifdef CERF_PDA
#define GPDR_VALUE              0x0C000090
#define GPSR_VALUE              0x0C000010
#define GPCR_VALUE              0x04000000
#define GRER_VALUE              0x00000000
#define GFER_VALUE              0x00000000
#define GEDR_VALUE              0xFFFFFFFF
#define GAFR_VALUE              0x00000000
#else
#define GPDR_VALUE              0x0320000F
#define GPSR_VALUE              0x03000000
#define GPCR_VALUE              0x0020000F
#define GRER_VALUE              0x00000000
#define GFER_VALUE              0x00000000
#define GEDR_VALUE              0xFFFFFFFF
#define GAFR_VALUE              0x00000000
#endif

#define UTSR0_x                 0x8005001C
#define UTDR_x                  0x80050014

#define RTTR                    0x90010008
#define RTTR_VALUE              32768

#define OSCR                    0x90000010
#define CLK_TO_10MS	        36864         // 3.686400 Mhz

#define PCFR                    0x90020010
#define PCFR_VALUE              3

#define ICMR                    0x90050004
#define ICMR_VALUE              0x00000000

#define ICLR                    0x90050008
#define ICLR_VALUE              0x00000000

#define PGSR                    0x90020018
#define PGSR_VALUE              0x00000000

#define MDREFR                  0xA000001C
#define MDCAS00                 0xA0000004
#define MDCAS01                 0xA0000008
#define MDCAS02                 0xA000000C
#define MDCAS20                 0xA0000020
#define MDCAS21                 0xA0000024
#define MDCAS22                 0xA0000028
#define MSC0                    0xA0000010
#define MSC1                    0xA0000014
#define MSC2                    0xA000002C
#define MECR                    0xA0000018
#define MDCNFG                  0xA0000000

#define PPCR                    0x90020014

#ifdef CERF_PDA
#define MDREFR_VALUE            0x80000161
#define MDCAS00_VALUE           0xAAAAAA9F
#define MDCAS01_VALUE           0xAAAAAAAA
#define MDCAS02_VALUE           0xAAAAAAAA
#define MDCAS20_VALUE           0xAAAAAA9F
#define MDCAS21_VALUE           0xAAAAAAAA
#define MDCAS22_VALUE           0xAAAAAAAA
#define MECR_VALUE              0x11FF7FE8
#define MSC0_VALUE              0x6E74FFF8
#define MSC1_VALUE              0xFFFFFFFF
#define MSC2_VALUE              0xFFFFFFFF
#define MDCNFG_VALUE            0x72647264
#else
#define MDREFR_VALUE            0x800002D1
#define MDCAS00_VALUE           0xAAAAAA9F
#define MDCAS01_VALUE           0xAAAAAAAA
#define MDCAS02_VALUE           0xAAAAAAAA
#define MDCAS20_VALUE           0xAAAAAA9F
#define MDCAS21_VALUE           0xAAAAAAAA
#define MDCAS22_VALUE           0xAAAAAAAA
#define MECR_VALUE              0x11FF7FE8
#define MSC0_VALUE              0x6E748377
#define MSC1_VALUE              0xFFFFFFFF
#define MSC2_VALUE              0xFFFFFFFF
#define MDCNFG_VALUE            0x72547254
#endif

// Power management
#define RCSR                    0x90030004
#define RCSR_SLEEP              0x00000008
#define PSPR                    0x90020008
#define PSSR                    0x90020004
#define PSSR_DH                 0x00000008
#define PSSR_PH                 0x00000010
#define PSSR_STATUS_MASK        0x00000007

#define SDRAM_BASE              0xC0000000

#define PPAR                    0x90060008
#define PPAR_VALUE              0x00000000

#define SDSR0                   0x80020080
#define SDSR1                   0x80020084
#define SDCR0                   0x80020060

#define SDSR0_VALUE             0x00000006
#define SDSR1_VALUE             0x00000010
#define SDCR0_VALUE             0x00000001

#endif //START_SA_H
