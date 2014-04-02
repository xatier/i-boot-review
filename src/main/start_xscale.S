//////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2001-2002 Intrinsyc Software Inc. All rights reserved.
//
// Module name:
//
//      start_xscale.S
//
// Description:
//
//      Bootstrap functions for Xscale. Sets up basic processor and memory
//      control registers, copies the bootloader into ram, and starts it.
//
// Author:
//
//      Mike Kirkland
//
// Created:
//
//      December 2001
//
////////////////////////////////////////////////////////////////////////////////

#include <start_xscale.h>

#ifdef VERBOSE_STARTUP
#define STARTUP_DEBUGGING 1
#endif

#define RAM_TEST_LENGTH 0x100000//0x1000000     // bytes of RAM to test

.text
.extern _ld_text_start
.extern _ld_text_and_data_size
.extern _ld_flash_base

.globl _start
_start:
        // Note that the entire bootloader has been linked assuming that
        // it is running out of RAM.  This means that care must be taken in
        // the first bit of code (that is run at a different address directly
        // out of flash) to not use absolute addresses without manually
        // adjusting them for the difference. Luckily, branch instructions
        // use relative addresses so they don't need to change.
        // Just don't be fooled by the fact that it works...

reset:
        // Reset entry point when we're running from flash.
        // We must copy ourselves to RAM before starting main.
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
        bl      init_xscale
        bl      init_gpio
        b       running_from_ram

exception:
run_rom:

#ifdef SLEEP_DEBUG
        // Do something visible at startup so we know we've awakened
        ldr     r0, =10
        bl      flash_led
#endif //SLEEP_DEBUG

        // Start running from ROM
        bl      init_xscale
        bl      init_gpio
        bl      init_sdram

	// Did we just wake up from deep sleep mode?
	// Wakeup from deep sleep mode
	ldr     r1, =RCSR
	ldr     r2, [r1]
	tst     r2, #RCSR_SLEEP
	beq     nosleep

        // Restart at wakeup vector
	ldr     r1, =PSPR
	ldr     r1, [r1]
	mov     pc, r1
	nop

nosleep:
        // Clear the sleep status register bits
	ldr     r1, =PSSR
	ldr     r2, =PSSR_STATUS_MASK
	str     r2, [r1]

#ifdef VERBOSE_STARTUP
	//when we're running from flash, we have to adjust addresses to their
	//location in flash, as opposed to their final location in ram
        ldr     r0, =hello
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
#endif

//	ldr	r12, =GPIO_BASE

//	ldr	r0, =0x00400000
//	str	r0, [r12, #GPCR0]

#ifdef STARTUP_MEMTEST
        ldr     r0, =start_test_ram
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
        ldr     r0, =0x55555555
        bl      test_ram

        ldr     r0, =start_walking_ones
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
        bl      walking_ones

        ldr     r0, =start_walking_zeroes
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
        bl      walking_zeroes

        ldr     r0, =start_streaming_ones
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
        bl      streaming_ones

        ldr     r0, =start_streaming_zeroes
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial
        bl      streaming_zeroes
#endif

	bl	copy_into_ram

#ifdef VERBOSE_STARTUP
	ldr	r0, =copied
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	ldr	r0, =running_from_ram
	bl	print_hex
	ldr	r0, =newline
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	ldr	r0, =_ld_text_and_data_size
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r0, =_ld_text_start
        bl      print_hex
        ldr      r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
	bl	wait_serial
#endif //VERBOSE_STARTUP

	ldr	r0, =running_from_ram
	mov	pc, r0

dies:
	b	reset

////////////////////////////////////////////////////////////////////////////////
// running_from_ram:
// PURPOSE:   Function jumped to after bootloader is loaded into ram. Prints a
//            status message and jumps to c_main.
// PARAMS:    None.
// REGISTERS: r0
// CALLS:     c_main
// RETURNS:   Does not return.
////////////////////////////////////////////////////////////////////////////////
running_from_ram:
	ldr	sp, =_ld_stack_address

//	ldr	r12, =GPIO_BASE
//	ldr	r0, =0x00000010     // LED is gpio 4
//	str	r0, [r12, #GPCR0]

        // We can't change the flash type while running out of flash, so do
        // it here when running out of RAM
        bl      init_flash

	//bl	print_hex
	//bl	wait_serial

	ldr	r0, =c_main
	mov	pc, r0

die:
	b	die

////////////////////////////////////////////////////////////////////////////////
// freq_change:
// PURPOSE:   Change the CPU frequency
// PARAMS:    r0 - CCCR value
// REGISTERS: r0, r1, r2
// RETURNS:   Nothing.
// NOTES:     This function conforms to the APCS and is callable from C as:
//            void freq_change(u32 cccr);
////////////////////////////////////////////////////////////////////////////////
.globl freq_change
freq_change:
        ldr     r1, =CLOCK_MANAGER_BASE
	str     r0, [r1, #CCCR]

        ldr     r1, =MEM_CTL_BASE
	ldr     r2,  [r1, #MDREFR]

	mov     r0, #3			// FCS, turbo

//#ifdef A1_COTULLA
	// Older revs need to be told to go into self refresh
	// B1's don't need this
	orr     r2,  r2,  #MDREFR_SLFRSH
	str	r2,  [r1, #MDREFR]
	bic     r2,  r2,  #MDREFR_SLFRSH
//#endif
	mcr     p14, 0, r0, c6, c0, 0			//freq change request

	// restart sdcke 0 / 1
	bic     r0,  r2,  #(MDREFR_E0PIN | MDREFR_E1PIN)
	str     r0,  [r1, #MDREFR]			//clear
	str     r2,  [r1, #MDREFR]			//restore

	// Generate refresh cycles for all banks
        ldr     r0, =SDRAM_BASE
	ldr     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]
	str     r2, [r0]

	// re-apply mdmrs
	ldr     r2,  =MDMRS_VALUE
	str     r2,  [r1, #MDMRS]

        // return
	mov     pc, lr

#ifdef STARTUP_MEMTEST
////////////////////////////////////////////////////////////////////////////////
// walking_ones:
// PURPOSE:   Does a walking ones test on RAM. It "walks" a one through each
//            cleared word in RAM. Eg:
//            10000000
//            01000000
//            00100000
//            00010000
//            00001000
//            00000100
//            00000010
//            00000001
// PARAMS:    None.
// REGISTERS: r0, r1, r2, r3, r6, r7, r8, r9
// CALLS:     print_string, print_hex, wait_serial
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
walking_ones:
	mov	r9, lr

	ldr	r1, =SDRAM_BASE
	ldr	r3, =RAM_TEST_LENGTH

walking_ones_next:
	ldr	r2, =0x10000000

walking_ones_loop:
	str	r2, [r1]
	ldr	r0, [r1]
	cmp	r0, r2
	bne	walking_ones_failed
        movs    r2, r2, LSR #1
	bne	walking_ones_loop
	add	r1, r1, #4
	sub	r3, r3, #1
	cmp	r3, #0
	bne	walking_ones_next

	ldr	r0, =ram_works
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial

	mov	pc, r9

walking_ones_failed:
	mov	r6, r1
	mov	r7, r2
	mov	r8, r0	

	ldr	r0, =ram_failed
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	mov	r0, r6
	bl	print_hex
	ldr	r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
	ldr	r0, =ram_value
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
	mov	r0, r8
	bl	print_hex
	ldr	r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
	bl	wait_serial

	mov	pc, r9


streaming_zeroes:
	mov	r9, lr

        ldr     r1, =SDRAM_BASE
        ldr     r3, =RAM_TEST_LENGTH

streaming_zeroes_next:
        ldr     r2, =0x7FFFFFFF
	ldr	r4, =32

streaming_zeroes_loop:
	str	r2, [r1]
	ldr	r0, [r1]
	cmp	r0, r2
	bne	streaming_zeroes_failed
	mov	r2, r2, LSR #1
	subs	r4, r4, #1
	bne	streaming_zeroes_loop
	add	r1, r1, #4
	subs	r3, r3, #1
	bne	streaming_zeroes_next

        ldr     r0, =ram_works
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9

streaming_zeroes_failed:
        mov     r6, r1
        mov     r7, r2
        mov     r8, r0

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
        mov     r0, r8
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9	


////////////////////////////////////////////////////////////////////////////////
// streaming_ones:
// PURPOSE:   Does a streaming ones test on RAM. It streams ones through each
//            word in RAM. Eg:
//            10000000
//            11000000
//            11100000
//            11110000
//            11111000
//            11111100
//            11111110
//            11111111
// PARAMS:    None.
// REGISTERS: r0, r1, r2, r3, r6, r7, r8, r9
// CALLS:     print_string, print_hex, wait_serial
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
streaming_ones:
        mov     r9, lr

        ldr     r1, =SDRAM_BASE
        ldr     r3, =RAM_TEST_LENGTH

streaming_ones_next:
        ldr     r2, =0x80000000

streaming_ones_loop:
        str     r2, [r1]
        ldr     r0, [r1]
        cmp     r0, r2
        bne     streaming_ones_failed
        mov     r2, r2, LSR #1
	orr	r2, r2, #0x80000000
	ldr	r13, =0xFFFFFFFF
        cmp     r2, r13
        bne     streaming_ones_loop
        add     r1, r1, #4
        subs    r3, r3, #1
        bne     streaming_ones_next

        ldr     r0, =ram_works
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9

streaming_ones_failed:
        mov     r6, r1
        mov     r7, r2
        mov     r8, r0

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
        mov     r0, r8
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9


walking_zeroes:
        mov     r9, lr

        ldr     r1, =SDRAM_BASE
        ldr     r3, =RAM_TEST_LENGTH
	ldr	r13, =0xFFFFFFFF

walking_zeroes_next:
        ldr     r2, =0x80000000

walking_zeroes_loop:
	eor	r12, r2, r13
        str     r12, [r1]
        ldr     r0, [r1]
        cmp     r0, r12
        bne     walking_zeroes_failed
        movs    r2, r2, LSR #1
        bne     walking_zeroes_loop
        add     r1, r1, #4
        subs    r3, r3, #1
        bne     walking_zeroes_next

        ldr     r0, =ram_works
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9

walking_zeroes_failed:
        mov     r6, r1
        mov     r7, r2
        mov     r8, r0

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
        mov     r0, r8
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        bl      wait_serial

        mov     pc, r9

#if 0
////////////////////////////////////////////////////////////////////////////////
//does not work
test_ram_mult:
	mov	r10, lr
	mov	r2, r0
	mov	r3, r0
	mov	r4, r0
	mov	r5, r0

	ldr	r0, =0x10000
	ldr	r1, =SDRAM_BASE

load_mult_loop:
	ldmia	r1!, {r2, r3, r4, r5}
	sub	r0, r0, #1
	cmp	r0, #0
	bne	load_mult_loop

        ldr     r0, =finished_loading
        ldr     r1, =_ld_text_start
        sub     r0, r0, #1
        bl      print_string
        bl      wait_serial

	ldr	r0, =0x10000

check_mult_loop:
	stmia	r1!, {r3, r4, r5, r6}
	cmp	r2, r3
	bne	failed_mult_check
	cmp	r2, r4
	bne	failed_mult_check
	cmp	r2, r5
	bne	failed_mult_check
	cmp	r2, r6
	bne	failed_mult_check

	sub	r0, r0, #1
	cmp	r0, #0
	bne	check_mult_loop

        ldr     r0, =ram_works
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

	mov	pc, r10

failed_mult_check:
	mov	r7, r1

        ldr     r0, =ram_failed
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        mov     r0, r7
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

//        ldr     r0, =ram_value
//        ldr     r1, =_ld_text_start
//        sub     r0, r0, r1
//        bl      print_string

	mov	pc, r10
#endif

////////////////////////////////////////////////////////////////////////////////
// test_ram
// PURPOSE:   Fills RAM with a constant (r0), and checks that RAM retains that
//            value
// PARAMS:    r0 - Constant to fill RAM with, also used as a counter
// REGISTERS: r0, r1, r3, r4, r6, r7, r8, r9, r10
// CALLS      print_hex, print_string, wait_serial
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
test_ram:
	mov	r9, lr
	mov	r3, r0

	ldr	r1, =SDRAM_BASE
	ldr	r0, =RAM_TEST_LENGTH
load_loop:
	str	r3, [r1]
	add	r1, r1, #4
	subs	r0, r0, #1
	bne	load_loop

	ldr	r0, =finished_loading
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial

	ldr	r1, =SDRAM_BASE
	ldr	r0, =RAM_TEST_LENGTH
	ldr	r7, =4

check_loop:
	subs	r7, r7, #1
	bne	no_reload
	ldr	r7, =4
no_reload:
	ldr	r4, [r1]
	cmp	r4, r3
	bne	failed_check
return_from_fail:
	add	r1, r1, #4
	subs	r0, r0, #1
	bne	check_loop

	ldr	r0, =ram_works
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	bl	wait_serial

	mov	pc, r9

failed_check:
	mov	r10, r3
	mov	r8, r4
	mov	r6, r0

	mov	r0, r7

	mov	r7, r1

	bl	print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

	ldr	r0, =ram_failed
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
	mov	r0, r7
	bl	print_hex
	ldr	r0, =newline
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string

        ldr     r0, =ram_value
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

	mov	r0, r8
	bl	print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

#if 0
        mov     r0, r6
        mov     r1, r7
        mov     r4, r8
        mov     r3, r10

        b       return_from_fail

#elif 1
	ldr	r1, =SDRAM_BASE
	ldr	r0, =RAM_TEST_LENGTH
	mov	r4, r8
	mov	r3, r10
	ldr	r7, =4

	b	return_from_fail
#endif
//	mov	pc, r9

#endif //STARTUP_MEMTEST


////////////////////////////////////////////////////////////////////////////////
// init_xscale
// PURPOSE:   Misc Xscale init.
// PARAMS:    None
// REGISTERS: r0, r1
// RETURNS:   Nothing
////////////////////////////////////////////////////////////////////////////////
init_xscale:
        mov     r1, lr          // save LR in case we switch CPU modes
	mrs	r0, cpsr
	bic	r0, r0, #0xDF
	orr	r0, r0, #0xD3
	msr	cpsr_c, r0      // disable interrupts and set SVC mode
        mov     lr, r1          // restore LR

        //enable access to all coprocessors
        ldr     r0, =0x2001
        mcr     p15, 0, r0, c15, c1, 0
        //Intel has macro call (CPWAIT) here, but they don't define it

        //disable MMU, instruction and data caches, and the write buffer
        ldr     r0, =0x00000078
        mcr     p15, 0, r0, c1, c0, 0
        //Intel has macro call (CPWAIT) here, but they don't define it

        //flush instruction and data tlb
        ldr     r0, =0x00000000
        mcr     p15, 0, r0, c8, c7, 0

        //flush instruction and data caches
        mcr     p15, 0, r0, c7, c7, 0

        //drain the write buffer
        mcr     p15, 0, r0, c7, c10, 4

        //intel does it, so I will too...
        nop
        nop
        nop

        //grant manager access to all domains
        mvn     r0, #0
        mcr     p15, 0, r0, c3, c0, 0

        ldr     r12, =INTERRUPT_CONTROL_BASE

        ldr     r0, =0x00000000
        str     r0, [r12, #ICMR]
        str     r0, [r12, #ICLR]

        // Set CPU clock speed. Believe it or not, the device "does not
        // support" the power on reset settings (see Intel errata #121).
        // The minimum CPU speed is therefore 200 MHz.
        ldr     r0, =CCCR_VALUE
        b       freq_change     // return from this function

        //mov     pc, lr

////////////////////////////////////////////////////////////////////////////////
// init_gpio:
// PURPOSE:   Initializes GPIO and FFUART registers. This function must be
//            run before anything may be printed to the debug port.
// PARAMS:    None.
// REGISTERS: r0, r1, r12
// CALLS:     Nothing.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
init_gpio:

// Initialize UART first so we can write debug messages right away

        ldr     r12, =FFUART_BASE

        ldr     r0, =FFLCR_VALUE_SETUP
        str     r0, [r12, #FFLCR]
        ldr     r0, =FFDLL_VALUE
        str     r0, [r12, #FFDLL]
        ldr     r0, =FFDLH_VALUE
        str     r0, [r12, #FFDLH]
        ldr     r0, =FFLCR_VALUE_USAGE
        str     r0, [r12, #FFLCR]
        ldr     r0, =FFFCR_VALUE
        str     r0, [r12, #FFFCR]
        ldr     r0, =FFMCR_VALUE
        str     r0, [r12, #FFMCR]

	ldr	r0, =FFISR_VALUE
	str	r0, [r12, #FFISR]

// If we are returning from sleep the GPIO registers will be rewritten
// by the kernel.  No harm done in setting these here.

        ldr     r12, =GPIO_BASE

        ldr     r0, =GAFR0L_VALUE
        str     r0, [r12, #GAFR0_L]
        ldr     r0, =GAFR0U_VALUE
        str     r0, [r12, #GAFR0_U]

        ldr     r0, =GAFR1L_VALUE
        str     r0, [r12, #GAFR1_L]
        ldr     r0, =GAFR1U_VALUE
        str     r0, [r12, #GAFR1_U]

        ldr     r0, =GAFR2L_VALUE
        str     r0, [r12, #GAFR2_L]
        ldr     r0, =GAFR2U_VALUE
        str     r0, [r12, #GAFR2_U]

        ldr     r0, =GPSR0_VALUE
        str     r0, [r12, #GPSR0]
        ldr     r0, =GPSR1_VALUE
        str     r0, [r12, #GPSR1]
        ldr     r0, =GPSR2_VALUE
        str     r0, [r12, #GPSR2]

        ldr     r0, =GPCR0_VALUE
        str     r0, [r12, #GPCR0]
        ldr     r0, =GPCR1_VALUE
        str     r0, [r12, #GPCR1]
        ldr     r0, =GPCR2_VALUE
        str     r0, [r12, #GPCR2]

        ldr     r0, =GPDR0_VALUE
        str     r0, [r12, #GPDR0]
        ldr     r0, =GPDR1_VALUE
        str     r0, [r12, #GPDR1]
        ldr     r0, =GPDR2_VALUE
        str     r0, [r12, #GPDR2]

#if 0
        ldr     r0, =GAFR0L_VALUE
        str     r0, [r12, #GAFR0_L]
        ldr     r0, =GAFR0U_VALUE
        str     r0, [r12, #GAFR0_U]

        ldr     r0, =GAFR1L_VALUE
        str     r0, [r12, #GAFR1_L]
        ldr     r0, =GAFR1U_VALUE
        str     r0, [r12, #GAFR1_U]

        ldr     r0, =GAFR2L_VALUE
        str     r0, [r12, #GAFR2_L]
        ldr     r0, =GAFR2U_VALUE
        str     r0, [r12, #GAFR2_U]
#endif
        // Clear the peripheral control register bits
	ldr     r1, =PSSR
	ldr     r0, =(PSSR_RDH | PSSR_PH)
	str     r0, [r1]

	//turn on the boot led
//        ldr     r0, =0x00000010
//        str     r0, [r12, #GPSR0]

        ldr     r12, =FFUART_BASE
        ldr     r0, =FFIER_VALUE
        str     r0, [r12, #FFIER]

	//clear the rx fifo errors
	ldr	r0, =FFFCR_VALUE
	and	r0, r0, #0x000000C3
	str	r0, [r12, #FFFCR]

#ifdef STARTUP_DEBUGGING
        b       print_gpioreg
#else
        mov     pc, lr
#endif

////////////////////////////////////////////////////////////////////////////////
// init_sdram:
// PURPOSE:   Initializes SDRAM registers.
// PARAMS:    None.
// REGISTERS: r0, r1, r2, r3, r4, r10, r11, r12
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
init_sdram:
	mov	r10, lr

        ldr     r12, =CLOCK_MANAGER_BASE

        ldr     r0, =CKEN_VALUE
        str     r0, [r12, #CKEN]
        ldr     r0, =OSCC_VALUE
        str     r0, [r12, #OSCC]

        //Step 1 in Intel's code
        ldr     r12, =MEM_CTL_BASE

        // We can't change the flash type while running out of flash,
        // so don't set MSC0 here because we likely are running from flash
        // at this point. Do it later, while running out of RAM, in init_flash.
        //ldr     r0, =MSC0_VALUE
        //str     r0, [r12, #MSC0]
        //ldr     r0, [r12, #MSC0]

        ldr     r0, =MSC1_VALUE
        str     r0, [r12, #MSC1]
        //Intel's code reads it back to make sure it works...
        ldr     r0, [r12, #MSC1]

        ldr     r0, =MSC2_VALUE
        str     r0, [r12, #MSC2]
        ldr     r0, [r12, #MSC2]

        ldr     r0, =MECR_VALUE
        str     r0, [r12, #MECR]

        ldr     r0, =MCMEM0_VALUE
        str     r0, [r12, #MCMEM0]

        ldr     r0, =MCMEM1_VALUE
        str     r0, [r12, #MCMEM1]

        ldr     r0, =MCATT0_VALUE
        str     r0, [r12, #MCATT0]

        ldr     r0, =MCATT1_VALUE
        str     r0, [r12, #MCATT1]

        ldr     r0, =MCIO0_VALUE
        str     r0, [r12, #MCIO0]

        ldr     r0, =MCIO1_VALUE
        str     r0, [r12, #MCIO1]

        //according to Intel's comments, we're extracting the DRI

	// Loading the MDREFR in a way that will make it configure
	// correctly is a multi-step process.  Please read Section
	// 6.12 of the PXA Developers Manual For more Details

	// 1.
	// The first step requires that we set K0RUN and E0PIN while
	// configuring DRI and clearing KXFREE.  All other values
	// MUST be left alone
        ldr     r0, =MDREFR_VALUE
        ldr     r3, [r12, #MDREFR]
        ldr     r1, =0xFFF
        and     r0, r0, r1

	// Make the DRI we read from MDREFR what MDREFR_VALUE says it is.
	// We also Free KXFREE the free running bits.
        bic     r3, r3, r1
	bic	r3, r3, #0x03800000
        orr     r3, r3, r0

        //Write it back
        str     r3, [r12, #MDREFR]


	// 2.
	// We don't have Synchronous Static Memory and don't want to
	// mess with SXCNFG or the like so we are leaving out this
	// step.

	// 3.
	// We don't bother to do this step as it does not seem to have
	// been done previously (actually maybe Self-Refresh Disable should
	// be here.)

        // 4.
	// Here we will setup the SDCLK's but WILL NOT enable them.  We
	// need to reload MDREFR for this.
        ldr     r0, =MDREFR_VALUE
	ldr	r1, =0xF6000	// Mask of SDCLK's settings minus EXPIN
	and	r0, r0, r1
        bic     r3, r3, r1
	orr	r3, r3, r0

#if 0	
#error wtf
        //this step is ifdefed for A1_Cotulla
        //(is this us? yes, but should we do this?)
        //set K1DB2 if setting banks 0-1 for 1/2 speed mem clock
#ifdef A1_COTULLA
//        orr     r3, r3, #0x00020000
	//set memory clock at full speed (100/100 Mhz for now)
	ldr	r11, =0xFFFDFFFF
	and	r3, r3, r11
#endif
#endif
        str     r3, [r12, #MDREFR]
	ldr	r3, [r12, #MDREFR]

	// 4.
        // Although I think that this should be at #3 It is here cause this
	// is where it was originally.  This will turn off Self-Refresh.
        bic     r3, r3,   #MDREFR_SLFRSH
        str     r3, [r12, #MDREFR]

	// 5.
	// Finally, we Enable the Various SDCLK's and let it run.
	// Also, enable the free-running clocks (not mentioned in the manual).
	ldr	r0, =MDREFR_VALUE
	ldr	r1, =0x03809000
	and	r0, r0, r1
	orr	r3, r3, r0
	str     r3, [r12, #MDREFR]
	nop
	nop

	//Step 4 in Intel's code
	ldr r0, =MDCNFG64_VALUE

        //disable all sdram banks
        bic     r0, r0, #(MDCNFG_DE0 | MDCNFG_DE1)
        bic     r0, r0, #(MDCNFG_DE2 | MDCNFG_DE3)

        //program banks 0/1 for 32 bit bus width
        bic     r0, r0, #0x00000004

	//test with 16 bit bus width
//	orr	r0, r0, #0x00000004

        //write MDCNFG, without enabling SDRAM banks
        str     r0, [r12, #MDCNFG]

        //Step 5 in Intel's code
        ldr     r0, =OSCR
	mov	r1, #0
	str	r1, [r0]

        //pause for approx 200 usecs
        ldr     r4, =0x300
sdram_dly:
        ldr     r1, [r0]
        cmp     r4, r1
        bgt     sdram_dly

        //Step 6 in Intel's code
        //turn everything off
        mov     r0, #0x78
        mcr     p15, 0, r0, c1, c0, 0


        //Step 7 in Intel's code
        //Access memory that has not been enabled for CBR refresh cycles (8)
	ldr	r0, =SDRAM_BASE
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]
        str     r0, [r0]

        //Step 8 is blank in Intel's code, though they mention dcache should
        //be enabled here if it is desired (we don't care)

//	ldr	r0, =0x0000007A
//	mcr	p15, 0, r0, c1, c0, 0

        //Step 9
        ldr     r0, [r12, #MDCNFG]

        //enable bank 0 (what about bank 1?)
        orr r0, r0, #MDCNFG_DE0
        str r0, [r12, #MDCNFG]

        //Step 10
        //write MDMRS again
        ldr     r0, =MDMRS_VALUE
        str     r0, [r12, #MDMRS]


        //Step 11
        //are we A1_Cotulla?
//#ifdef A1_COTULLA
        ldr     r0, [r12, #MDREFR]

        //enable auto-power down
//        orr     r0, r0, #0x00100000
	//disable auto-power down
	ldr	r11, =0xFFEFFFFF
	and	r0, r0, r11

        //write back MDREFR
        str     r0, [r12, #MDREFR]
//#endif

#ifdef STARTUP_DEBUGGING
        bl      print_memreg
#endif
        mov     pc, r10

////////////////////////////////////////////////////////////////////////////////
// init_flash:
// PURPOSE:   Initializes Flash registers.
// PARAMS:    None.
// REGISTERS: r0, r12
// RETURNS:   Nothing.
// NOTES:     The flash type can't be set while running out of flash, so this
//            function must be called while running out of RAM.
////////////////////////////////////////////////////////////////////////////////
init_flash:
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, =MSC0_VALUE
        str     r0, [r12, #MSC0]
        //Intel's code reads it back to make sure it works...
        ldr     r0, [r12, #MSC0]
        mov     pc, lr

////////////////////////////////////////////////////////////////////////////////
// print_string:
// PURPOSE:   Prints a null terminated string to the debug port.
// PARAMS:    r0 - Address of string.
// REGISTERS: r0, r1, r12
// CALLS:     Nothing.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
print_string:
        ldr     r12, =FFUART_BASE

print_loop:
        ldr     r1, [r12, #FFLSR]
        tst     r1, #0x00000020
        beq     print_loop

        ldrsb   r1, [r0]
        cmp     r1, #0x00
        beq     null_char
        add     r0, r0, #1
        str     r1, [r12, #FFTHR]
        b       print_loop

null_char:
        mov     pc, lr

#if defined(VERBOSE_STARTUP) || defined(STARTUP_DEBUGGING) || defined(STARTUP_MEMTEST)
////////////////////////////////////////////////////////////////////////////////
// print_byte:
// PURPOSE:   Prints a character to the debug port
// PARAMS:    None.
// REGISTERS: r0, r11, r12
// CALLS:     Nothing.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
.globl print_byte
print_byte:
        ldr     r12, =FFUART_BASE
print_byte_loop:
        ldr     r11, [r12, #FFLSR]
        tst     r11, #0x00000020
        beq     print_byte_loop

        str     r0, [r12, #FFTHR]
        mov     pc, lr

////////////////////////////////////////////////////////////////////////////////
// print_hex:
// PURPOSE:   Prints a 32 bit hex value to the debug port.
// PARAMS:    r0
// REGISTERS: r0, r1, r2, r3
// CALLS:     print_byte
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
.globl print_hex
print_hex:
        mov     r3, lr
        mov     r2, r0
        ldr     r1, =28         // bits to shift for first nibble
loop_hex:
        mov     r0, r2, LSR r1
        and     r0, r0, #0xF
        cmp     r0, #9
        bhi     alpha
        add     r0, r0, #'0'
        b       done_hex
alpha:
        add     r0, r0, #('A' - 10)
done_hex:
        bl      print_byte
        subs    r1, r1, #4
        bpl     loop_hex
        mov     pc, r3
#endif

////////////////////////////////////////////////////////////////////////////////
// wait_serial:
// PURPOSE:   Wait for all serial characters to be sent.
// PARAMS:    None.
// REGISTERS: r0, r12
// CALLS:     None.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
.globl wait_serial
wait_serial:
        ldr     r12, =FFUART_BASE
        ldr     r0, [r12, #FFLSR]
        tst     r0, #0x00000040
        beq     wait_serial

	mov	pc, lr

#if 0
////////////////////////////////////////////////////////////////////////////////
// spin:
// PURPOSE:   Sit in a busy loop
// PARAMS:    None.
// REGISTERS: r0
// CALLS:     None.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
spin:
        subs    r0, r0, #1
        bne     spin
        mov     pc, lr
#endif

#ifdef SLEEP_DEBUG
////////////////////////////////////////////////////////////////////////////////
// flash_led:
// PURPOSE:   Blinks the led
// PARAMS:    r0 - Number of times to blink led.
// REGISTERS: r0, r4, r6, r7
// CALLS:     None.
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////

/* wait for a 0.25 seconds (at 100MHz and 3 instructions) */
#define BLINK_WAIT      0x1fca05

.globl flash_led
flash_led:
        ldr     r6, =GPIO_BASE      // GPIO BASE
        ldr     r7, =0x00000010     // LED is gpio 4

/* set direction to output */
        ldr     r4, [r6, #GPDR0]
        orr     r4, r4, r7
        str     r4, [r6, #GPDR0]

loop_led:
/* turn on LED */
        str     r7, [r6, #GPSR0]

/* wait */
        ldr     r4, =BLINK_WAIT
wait_on:
        sub     r4, r4, #1
        cmp     r4, #0
        bne     wait_on

/* turn off LED */
        str     r7, [r6, #GPCR0]

/* wait */
        ldr     r4, =BLINK_WAIT
wait_off:
        sub     r4, r4, #1
        cmp     r4, #0
        bne     wait_off

/* decrement, loop count */
        sub     r0, r0, #1
        cmp     r0, #0
        bne     loop_led

/* wait */
        ldr     r4, =4*BLINK_WAIT
wait_final:
        sub     r4, r4, #1
        cmp     r4, #0
        bne     wait_final

        mov pc, lr
#endif

#ifdef STARTUP_DEBUGGING
////////////////////////////////////////////////////////////////////////////////
// print_gpioreg:
// PURPOSE:   Prints values of GPIO registers.
// PARAMS:    None.
// REGISTERS: r0, r1, r6, r14
// CALLS:     print_string, print_hex
// RETURNS:   Nothing.
////////////////////////////////////////////////////////////////////////////////
print_gpioreg:
	mov	r6, lr
	ldr	r14, =GPIO_BASE

	ldr	r0, =gplr0_string
	ldr	r1, =_ld_text_start
	sub	r0, r0, r1
	bl	print_string
        ldr     r14, =GPIO_BASE
	ldr	r0, [r14, #GPLR0]
	bl	print_hex
	ldr	r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gplr1_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GPLR1]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gplr2_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GPLR2]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gpdr0_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GPDR0]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gpdr1_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GPDR1]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gpdr2_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GPDR2]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr0l_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR0_L]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr1l_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR1_L]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr2l_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR2_L]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr0u_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR0_U]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr1u_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR1_U]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

        ldr     r0, =gafr2u_string
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string
        ldr     r14, =GPIO_BASE
        ldr     r0, [r14, #GAFR2_U]
        bl      print_hex
        ldr     r0, =newline
        ldr     r1, =_ld_text_start
        sub     r0, r0, r1
        bl      print_string

	mov	pc, r6

////////////////////////////////////////////////////////////////////////////////
//prints out memory controller register values
////////////////////////////////////////////////////////////////////////////////
print_memreg:
        //save lr
        mov     r6, lr

        adr     r0, mdcnfg_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MDCNFG]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mdrefr_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MDREFR]
        bl      print_hex
nop
        adr     r0, newline
        bl      print_string

        adr     r0, mdmrs_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MDMRS]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, msc0_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MSC0]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, msc1_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MSC1]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, msc2_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MSC2]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mecr_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MECR]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, sxcnfg_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #SXCNFG]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcmem0_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCMEM0]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcmem1_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCMEM1]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcatt0_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCATT0]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcatt1_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCATT1]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcio0_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCIO0]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, mcio1_string
        bl      print_string
        ldr     r12, =MEM_CTL_BASE
        ldr     r0, [r12, #MCIO1]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        adr     r0, cccr_string
        bl      print_string
        ldr     r12, =CLOCK_MANAGER_BASE
        ldr     r0, [r12, #CCCR]
        bl      print_hex
        adr     r0, newline
        bl      print_string

        mov     pc, r6
#endif //STARTUP_DEBUGGING

////////////////////////////////////////////////////////////////////////////////
// copy_into_ram:
// PURPOSE:   Copies the bootloader into ram, so that it may be run from there.
//            Without doing this, flash cannot be written to.
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

.globl inc_led
inc_led:
	mov	pc, lr

.align 4
newline:
        .string "\r\n"

#ifdef VERBOSE_STARTUP
.align 4
hello:
        //.string "Hello, World!\r\n"
        .string "+"

.align 4
copied:
        .string "Copied bootloader into RAM.\r\n"
#endif

#if 0
.align 4
configram:
        .string "Configured SDRAM\r\n"

.align 4
branched:
        .string "Branched to c_main (so we shouldn't be here)\r\n"

.align 4
memworks:
        .string "Memory works!\r\n"
#endif

#ifdef STARTUP_DEBUGGING
.align 4
mdcnfg_string:
        .string "MDCNFG: "

.align 4
mdrefr_string:
        .string "MDREFR: "

.align 4
msc0_string:
        .string "MSC0: "

.align 4
msc1_string:
        .string "MSC1: "

.align 4
msc2_string:
        .string "MSC2: "

.align 4
mecr_string:
        .string "MECR: "

.align 4
sxcnfg_string:
        .string "SXCNFG: "

.align 4
mcmem0_string:
        .string "MCMEM0: "

.align 4
mcmem1_string:
        .string "MCMEM1: "

.align 4
mcatt0_string:
        .string "MCATT0: "

.align 4
mcatt1_string:
        .string "MCATT1: "

.align 4
mcio0_string:
        .string "MCIO0: "

.align 4
mcio1_string:
        .string "MCIO1: "

.align 4
cccr_string:
        .string "CCCR: "

.align 4
mdmrs_string:
	.string "MDMRS: "

.align 4
gpdr0_string:
	.string "GPDR0: "

.align 4
gpdr1_string:
        .string "GPDR1: "

.align 4
gpdr2_string:
        .string "GPDR2: "

.align 4
gplr0_string:
        .string "GPLR0: "

.align 4
gplr1_string:
        .string "GPLR1: "

.align 4
gplr2_string:
        .string "GPLR2: "

.align 4
gafr0l_string:
	.string "GAFR0_L: "

.align 4
gafr1l_string:
        .string "GAFR1_L: "

.align 4
gafr2l_string:
        .string "GAFR2_L: "

.align 4
gafr0u_string:
        .string "GAFR0_U: "

.align 4
gafr1u_string:
        .string "GAFR1_U: "

.align 4
gafr2u_string:
        .string "GAFR2_U: "

#endif //STARTUP_DEBUGGING

#ifdef STARTUP_MEMTEST

.align 4
ram_works:
	.string "RAM test passed.\r\n"

.align 4
ram_failed:
	.string "RAM test failed at address: "

.align 4
ram_value:
	.string "Value read from RAM was: "

.align 4
finished_loading:
	.string "Finished loading\r\n"

.align 4
start_test_ram:
	.string "Starting RAM test\r\n"

.align 4
start_walking_ones:
	.string "Starting walking ones test\r\n"

.align 4
start_streaming_zeroes:
	.string "Starting streaming zeroes test\r\n"

.align 4
start_streaming_ones:
	.string "Starting streaming ones test\r\n"

.align 4
start_walking_zeroes:
	.string "Starting walking zeroes test\r\n"

#endif //STARTUP_MEMTEST

#ifdef SLEEP_DEBUG

.align 4
not_waking_from_sleep:
	.string "Booting from hard reset\r\n"

.align 4
waking_from_sleep:
	.string "Waking from sleep\r\n"

#endif //SLEEP_DEBUG
