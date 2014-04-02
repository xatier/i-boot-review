//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001-2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      start_sa.S
//
// Description:
//
//      Bootstrap functions for SA1110. Sets up basic processor and memory
//      control registers, copies the bootloader into RAM, and starts it.
//
// Author:
//
//
// Created:
//
//
////////////////////////////////////////////////////////////////////////////////

#define ASM
#include <serial.h>
#include <start_sa.h>

#define RAM_TEST_LENGTH 0x100000//0x1000000     // bytes of RAM to test

.text
.extern _ld_text_start
.extern _ld_text_and_data_size
.extern _ld_flash_base

.globl _start
_start:
reset:
        // Note that the entire bootloader has been linked assuming that
        // it is running out of RAM.  This means that care must be taken in
        // the first bit of code (that is run at a different address directly
        // out of flash) to not use absolute addresses without manually
        // adjusting them for the difference. Luckily, branch instructions
        // use relative addresses so they don't need to change.
        // Just don't be fooled by the fact that it works...

        // Reset entry point when we're starting from flash.
        b       run_rom

        // Undefined instruction vector
        // We are using this as an alternate entry point when running out of RAM
        // We should transition to using _start+0x20 as the offset instead.
        b       run_ram

        // Software interrupt (SWI) vector
        b       exception

        // Prefetch abort vector
        b       exception

        // Data abort vector
        b       exception

        // Reserved vector
        b       exception

        // IRQ vector
        b       exception

        // FIRQ vector
        b       exception

run_ram:
        // _start+0x20: Entry point when we're already in RAM.
        // Simplified initialization, since we don't need to copy ourselves
        // to RAM. We do need to ensure that interrupts are off and GPIO is
        // what we expect.
	bl	init_strongarm
	bl	init_gpio
	
    // xatier: oh, ya!
	//We're alive! turn on the LEDs
	ldr	r12, =GPIO_BASE
	ldr	r0, =0x0000000F
	str	r0, [r12, #GPSR]

	bl	init_serial
        b       running_from_ram

exception:
run_rom:
	bl	init_strongarm
	bl	init_gpio
	
    // xatier: oh, ya!
	//We're alive! turn on the LEDs
	ldr	r12, =GPIO_BASE
	ldr	r0, =0x0000000F
	str	r0, [r12, #GPSR]

	//ldr	r0, =0x10000    // allow serial port FIFO to empty
	//bl	delay

	bl	init_sdram
	bl	init_serial

	// Did we just wake up from deep sleep mode?
	// Wakeup from deep sleep mode
	ldr     r1, =RCSR
	ldr     r2, [r1]
	tst     r2, #RCSR_SLEEP
	beq     nosleep

	ldr     r1, =PSPR
	ldr     r1, [r1]
	mov     pc, r1
	nop

nosleep:
	// Release peripheral status bits
	ldr     r1, =PSSR
	ldr     r2, =PSSR_STATUS_MASK
	str     r2, [r1]

#ifdef VERBOSE_STARTUP
	//when we're running from flash, we have to adjust addresses to their
	//location in flash, as opposed to their final location in RAM
        ldr     r0, =hello
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
#endif

#ifdef STARTUP_MEMTEST
	ldr     r0, =start_walking_ones
	ldr     r1, =_ld_text_start
	sub     r0, r0, r1
	bl      print_string
	bl      walking_ones
#endif //STARTUP_MEMTEST

	// Copy the whole bootloader into RAM and restart it from there
	bl	copy_into_ram

	ldr	r0, =running_from_ram
	mov	pc, r0

running_from_ram:
	bl	inc_led

	ldr	sp, =_ld_stack_address
	ldr	r0, =c_main
	mov	pc, r0

.align 4
.globl inc_led
//The PDA only has one LED, so we can't count with it.
#ifdef CERF_PDA
inc_led:
	mov	pc, lr
#else
inc_led:
	ldr	r12, =GPIO_BASE
	ldr	r0, [r12, #GPLR]
	ldr	r1, =0x0000000F
	and	r0, r0, r1
	eor	r0, r0, r1
	add	r0, r0, #1
	eor	r0, r0, r1
	and	r0, r0, r1
	str	r1, [r12, #GPCR]
	str	r0, [r12, #GPSR]

	mov	pc, lr
#endif

init_gpio:
	ldr	r12, =GPIO_BASE
	
	ldr	r0, =GPDR_VALUE
	str	r0, [r12, #GPDR]

	ldr	r0, =GPSR_VALUE
	str	r0, [r12, #GPSR]

	ldr	r0, =GPCR_VALUE
	str	r0, [r12, #GPCR]

	// Release peripheral hold
	ldr     r1, =PSSR
	ldr     r2, =PSSR_PH
	str     r2, [r1]

	mov	pc, lr


init_strongarm:
        mov     r1, lr          // save LR in case we switch CPU modes
	mrs	r0, cpsr
	bic	r0, r0, #0xDF
	orr	r0, r0, #0xD3
	msr	cpsr_c, r0      // disable interrupts and set SVC mode
        mov     lr, r1          // restore LR

	ldr	r0, =0
	mcr	p15, 0, r0, c1, c0, 0
	mcr	p15, 0, r0, c8, c7, 0
	mcr	p15, 0, r0, c7, c7, 0

	ldr	r12, =RTTR
	ldr	r0, =RTTR_VALUE
	str	r0, [r12]

	ldr	r12, =PCFR
	ldr	r0, =PCFR_VALUE
	str	r0, [r12]

	ldr	r12, =GPIO_BASE

	ldr	r0, =GRER_VALUE
	str	r0, [r12, #GRER]

	ldr	r0, =GFER_VALUE
	str	r0, [r12, #GFER]

	ldr	r0, =GEDR_VALUE
	str	r0, [r12, #GEDR]

	ldr	r0, =GAFR_VALUE
	str	r0, [r12, $GAFR]

	ldr	r12, =ICMR
	ldr	r0, =ICMR_VALUE
	str	r0, [r12]

	ldr	r12, =ICLR
	ldr	r0, =ICLR_VALUE
	str	r0, [r12]

	ldr	r12, =PGSR
	ldr	r0, =PGSR_VALUE
	str	r0, [r12]

	ldr	r12, =PPCR
	ldr	r0, =0x9
	str	r0, [r12]

	mov	pc, lr


// xatier: initialize the serial port, at baud rate: 115200
init_serial:
	ldr	r12, =PPAR
	ldr	r0, =PPAR_VALUE
	str	r0, [r12]

	ldr	r12, =SDSR0
	ldr	r0, =SDSR0_VALUE
	str	r0, [r12]

	ldr	r12, =SDSR1
	ldr	r0, =SDSR1_VALUE
	str	r0, [r12]

	ldr	r12, =SDCR0
	ldr	r0, =SDCR0_VALUE
	str	r0, [r12]

	ldr	r12, =SERIAL_UTCR0_ASM
	ldr	r0, =0x00000008
	str	r0, [r12]

	ldr	r12, =SERIAL_UTCR1_ASM
	ldr	r0, =0x00000000
	str	r0, [r12]

	ldr	r12, =SERIAL_UTCR2_ASM
	ldr	r0, =SERIAL_BAUD_38400
	str	r0, [r12]

	ldr	r12, =SERIAL_UTCR3_ASM
	ldr	r0, =0x00000003
	str	r0, [r12]

	mov	pc, lr

init_sdram:
	ldr	r12, =MDREFR
	ldr	r0, =MDREFR_VALUE
	str	r0, [r12]

	ldr	r12, =MDCAS00
	ldr	r0, =MDCAS00_VALUE
	str	r0, [r12]

        ldr     r12, =MDCAS01
	ldr     r0, =MDCAS01_VALUE
	str     r0, [r12]

        ldr     r12, =MDCAS02
	ldr     r0, =MDCAS02_VALUE
	str     r0, [r12]

        ldr     r12, =MDCAS20
	ldr     r0, =MDCAS20_VALUE
	str     r0, [r12]

        ldr     r12, =MDCAS21
	ldr     r0, =MDCAS21_VALUE
	str     r0, [r12]

        ldr     r12, =MDCAS21
	ldr     r0, =MDCAS21_VALUE
	str     r0, [r12]

	ldr	r12, =MSC0
	ldr	r0, =MSC0_VALUE
	str	r0, [r12]

	ldr	r12, =MSC1
	ldr	r0, =MSC1_VALUE
	str	r0, [r12]

	ldr	r12, =MSC2
	ldr	r0, =MSC2_VALUE
	str	r0, [r12]

	ldr	r12, =MECR
	ldr	r0, =MECR_VALUE
	str	r0, [r12]

	ldr	r12, =MDREFR
	ldr	r0, [r12]
	//set K1RUN bit
	orr	r0, r0, #0x00200000
	str	r0, [r12]
	//clear SLFRSH bit
	bic	r0, r0, #0x80000000
	str	r0, [r12]
	//set E1PIN bit
	orr	r0, r0, #0x00100000
	str	r0, [r12]

	ldr	r12, =MDCNFG
	ldr	r0, =MDCNFG_VALUE
	str	r0, [r12]

	//clear the DRAM control hold bit
	ldr	r12, =PSSR
	ldr	r0, =PSSR_DH
	str	r0, [r12]

	//trigger 8 refresh cycles
	ldr	r12, =SDRAM_BASE
	ldr	r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]
        ldr     r0, [r12]

	//enable SDRAM banks
	ldr	r12, =MDCNFG
	ldr	r0, [r12]
	ldr	r1, =0x00030003
	orr	r0, r0, r1
	str	r0, [r12]

	//wait for SDRAM to enable
	ldr	r0, =0x400
wait_sdram:
	subs	r0, r0, #1
	bne	wait_sdram

	mov	pc, lr


die:
	b	die


////////////////////////////////////////////////////////////////////////////////
// copy_into_ram:
// PURPOSE:   Copies the bootloader into ram, so that it may be run from there.
//            Flash cannot be read while it is being written, so code which
//            writes to flash must not be run from flash.
// PARAMS:    None.
// REGISTERS: r0, r1, r2, r3, r8, r12
// CALLS:     Nothing
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
copy_into_ram:
	mov	r8, lr


	ldr	r0, =0
	ldr	r1, =_ld_text_start
	ldr	r2, =_ld_text_and_data_size
//	mov	r2, r2, LSR #2
//	add	r2, r2, #1

copy_loop:
	ldr	r3, [r0]
	str	r3, [r1]
	add	r0, r0, #4
	add	r1, r1, #4
	subs	r2, r2, #4
	bne	copy_loop

	mov	pc, r8

delay:
	subs	r0, r0, #1
	bgt	delay
	mov	pc, lr

////////////////////////////////////////////////////////////////////////////////
// freq_change:
// PURPOSE:   Change the CPU frequency and enable clock switching
// PARAMS:    r0 - CCF value
// REGISTERS: r0, r1
// RETURNS:   Nothing.
// NOTES:     This function conforms to the APCS and is callable from C as:
//            void freq_change(u32 ccf);
////////////////////////////////////////////////////////////////////////////////
.globl freq_change
freq_change:
        ldr     r1, =PPCR
	str     r0, [r1]

        // Enable clock switching
        ldr     r1, =0x00000000
	mcr     p15, 0, r1, c15, c1, 2

        // return
	mov     pc, lr

////////////////////////////////////////////////////////////////////////////////
// print_byte:
// PURPOSE:   Prints a character to the debug port
// PARAMS:    None.
// REGISTERS: r0, r1
// CALLS:     Nothing.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
#if defined(STARTUP_MEMTEST) || defined(VERBOSE_STARTUP)
print_byte:
	ldr	r1, =UTSR0_x
	ldr     r1, [r1]
	ands    r1, r1, #0x01
	beq     print_byte

	ldr	r1, =UTDR_x
	str     r0, [r1]

	mov	pc, lr

////////////////////////////////////////////////////////////////////////////////
// print_hex:
// PURPOSE:   Prints a 32 bit hex value to the debug port.
// PARAMS:    r0
// REGISTERS: r0, r1, r2, r3
// CALLS:     print_byte
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
print_hex:
        mov     r3, lr
	mov     r2, r0
        ldr     r1, =28         // bits to shift for first nibble
loop_hex:
	mov     r0, r2, LSR r1
	and     r0, r0, #0xF
	cmp     r0, #9
	addhi   r0, r0, #('A' - '0' - 10)
	add     r0, r0, #'0'
	bl      print_byte
	subs    r1, r1, #4
	bpl     loop_hex
	mov     pc, r3

print_string:
	mov     r13, lr
	mov     r2, r0
prs1:
	ldrsb   r0, [r2]
	add     r2, r2, #0x01
	ands    r0, r0, #0xFF
	beq     prs2
	bl      print_byte
	b       prs1
prs2:
	mov	pc, r13
#endif

#ifdef STARTUP_MEMTEST
walking_ones:
        mov     r9, lr

	ldr     r1, =SDRAM_BASE
	ldr     r3, =RAM_TEST_LENGTH
	
walking_ones_next:
	ldr     r2, =0x80000000

walking_ones_loop:
	str     r2, [r1]
	//ldr     r5, [r1]
	cmp     r5, r2
	bne     walking_ones_failed
	mov     r2, r2, LSR #1
	cmp     r2, #0
	bne     walking_ones_loop
	add     r1, r1, #4
	subs    r3, r3, #1
	bne     walking_ones_next
	
	ldr	r0, =ram_works
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string

	mov     pc, r9

walking_ones_failed:
	mov     r6, r1
	mov     r7, r2
	mov     r8, r5

	ldr     r0, =ram_failed
	ldr     r1, =_ld_text_start
	sub     r0, r0, r1
	bl      print_string
	mov     r0, r6
	bl      print_hex
	ldr     r0, =newline
	ldr     r1, =_ld_text_start
	sub     r0, r0, r1
	bl      print_string
	ldr     r0, =ram_value
	ldr     r1, =_ld_text_start
	sub     r0, r0, r1
	bl      print_string
	mov     r0, r5
	bl      print_hex
	ldr     r0, =newline
	ldr     r1, =_ld_text_start
	sub     r0, r0, r1
	bl      print_string

	mov     pc, r9

#endif //STARTUP_MEMTEST

.align 4
newline:
        .string "\r\n"

.align 4
hello:
        //.string "Hello, World!\r\n"
        .string "+"

.align 4
start_walking_ones:
	.string "Starting walking ones test\r\n"

.align 4
ram_works:
	.string "RAM test passed.\r\n"

.align 4
ram_failed:
	.string "RAM test failed at address: "

.align 4
ram_value:
	.string "Value read from RAM was: "

