/*
 * zdi.c
 *
 *  Created on: Jan 10, 2021
 *      Author: bje
 */

#include <stdint.h>

#include "stm32l0xx.h"
#include "zdi.h"

/**
 * Write a value to a ZDI register.
 *
 * Hopefully without melting down an expensive and hard to solder CPU.
 */
void zdi_write(uint8_t reg, uint8_t data)
{
    volatile uint32_t *otyper = ((volatile uint32_t *)0x50000004);
    volatile uint32_t *bsrr = ((volatile uint32_t *)0x50000018);
    uint32_t zcl_hi = 1 << 5;   // PA5 bit set
    uint32_t zcl_lo = 1 << 21;  // PA5 bit reset
    uint32_t zda_hi = 1 << 4;   // PA4 bit set
    uint32_t zda_lo = 1 << 20;  // PA4 bit reset

    uint32_t otype = *otyper;
    uint32_t zda_pp = otype & ~(1<<4);
    uint32_t zda_od = otype | (1<<4);

    /* Set ZDA high, ZCL high, then ZDA to push-pull */
    *bsrr = zda_hi | zcl_hi;
    *otyper = zda_pp;

    /* bsrr_bits has the reset bit pattern in the top half-word and the set bit pattern in the bottom */
    uint32_t reg_bsrr_bits = (~reg) << 16 | reg;
    uint32_t data_bsrr_bits = (~data) << 16 | data;

    /* bsrr_mask selects just PA4's set/reset bits */
    uint32_t bsrr_mask = zda_hi | zda_lo;

    /* Turn off interrupts, if enabled */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /*   C     c     C     c     C     c     C     c
     *    _____       _____       _____       _____
     * __/     \_____/     \_____/     \_____/     \_____
     *           D N   S A   D           D           D
     *
     * C - write clock high
     * c - write clock low
     * D - write data bit
     * S - shift data byte
     * A - AND data byte with mask
     * N - NOP
     *
     * SAcDNC
     *
     */

    asm volatile (
    		/* -masm-syntax-unified would avoid needing this */
    		".syntax	unified\n\t"

            /* start signal */
    		"mov	r5, %[reg]\n\t"					// r5 holds the value being sent
            "str	%[zda_lo], [%[bsrr]]\n\t"

            /* bit 6 of register */
            "lsrs	r4, r5, #2\n\t"             	// shift bit 6 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 5 of register */
            "lsrs	r4, r5, #1\n\t"             	// shift bit 5 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 4 of register */
            "lsrs	r4, r5, #0\n\t"             	// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 3 of register */
            "lsls	r4, r5, #1\n\t"             	// shift bit 3 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 2 of register */
            "lsls	r4, r5, #2\n\t"             	// shift bit 2 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 1 of register */
            "lsls	r4, r5, #3\n\t"             	// shift bit 1 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 0 of register */
            "lsls	r4, r5, #4\n\t"             	// shift bit 0 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "movs	r4, #128\n\t"                   // prepare the write bit: set bit 7
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* write bit: reset ZDA */
            "lsls	r4, r4, #13\n\t"                // shift bit 7 to bit 20
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* single-bit separator: keep low */
            "nop\n\t"                               // delay to balance duty cycle
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "nop\n\t"                               // delay to balance duty cycle
    		"mov	r5, %[data]\n\t"				// r5 holds the value being sent
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 7 of data */
            "lsrs	r4, r5, #3\n\t"            		// shift bit 7 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 6 of data */
            "lsrs	r4, r5, #2\n\t"            		// shift bit 6 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 5 of data */
            "lsrs	r4, r5, #1\n\t"            		// shift bit 5 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 4 of data */
            "lsrs	r4, r5, #0\n\t"            		// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 3 of data */
            "lsls	r4, r5, #1\n\t"            		// shift bit 3 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 2 of data */
            "lsls	r4, r5, #2\n\t"            		// shift bit 2 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 1 of data */
            "lsls	r4, r5, #3\n\t"            		// shift bit 1 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* bit 0 of data */
            "lsls	r4, r5, #4\n\t"            		// shift bit 0 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "movs	r4, #16\n\t"                    // prepare the separator bit: set bit 4
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

            /* single-bit separator: drive high */
            "nop\n\t"                               // delay to balance duty cycle
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_lo], [%[bsrr]]\n\t"       // drive clock LOW
            "str	r4, [%[bsrr]]\n\t"              // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[bsrr]]\n\t"       // drive clock HIGH

    		/* wait it out and clock low once more */

            : /* no outputs */
            : [reg]     "r" (reg_bsrr_bits),
              [data]    "r" (data_bsrr_bits),
              [bsrr]    "l" (bsrr),
              [zda_lo]  "l" (zda_lo),
              [zcl_lo]  "l" (zcl_lo),
              [zcl_hi]  "l" (zcl_hi),
              [mask]    "l" (bsrr_mask)
            : "r4", "r5", "cc", "memory"
    );

    *otyper = zda_od;
    *bsrr = zcl_hi;

    /* Restore interrupts to previous state */
    __set_PRIMASK(primask);
}

uint8_t zdi_read(uint8_t reg)
{
    volatile uint32_t *otyper = ((volatile uint32_t *)0x50000004);
    volatile uint32_t *bsrr = ((volatile uint32_t *)0x50000018);
    uint32_t zcl_hi = 1 << 5;   // PA5 bit set
    uint32_t zcl_lo = 1 << 21;  // PA5 bit reset
    uint32_t zda_hi = 1 << 4;   // PA4 bit set
    uint32_t zda_lo = 1 << 20;  // PA4 bit reset

    uint32_t otype = *otyper;
    uint32_t zda_pp = otype & ~(1<<4);
    uint32_t zda_od = otype | (1<<4);

    /* Set ZDA high, ZCL high, then ZDA to push-pull */
    *bsrr = zda_hi | zcl_hi;
    *otyper = zda_pp;

    /* bsrr_bits has the reset bit pattern in the top half-word and the set bit pattern in the bottom */
    uint32_t reg_bsrr_bits = (~reg) << 16 | reg;

    /* bsrr_mask selects just PA4's set/reset bits */
    uint32_t bsrr_mask = zda_hi | zda_lo;

    uint32_t result = 0;

    /* Turn off interrupts, if enabled */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /*   C     c     C     c     C     c     C     c
     *    _____       _____       _____       _____
     * __/     \_____/     \_____/     \_____/     \_____
     *     N R   S M   O R   S M   O R
     *
     * clock high, or input, read port, clock low, shift input, mask input, clock high, or input, ...
	 */

    asm volatile (
    		/* -masm-syntax-unified would avoid needing this */
    		".syntax	unified\n\t"

            /* start signal */
    		"movs	r4, #16\n\t"					// ZDA set
    		"lsls	r4, r4, #16\n\t"				// shift to ZDA reset
            "str	r4, [%[port], #24]\n\t"			// *bsrr = ZDA reset
    		"mov	r5, %[reg]\n\t"					// r5 holds the value being sent

            /* bit 6 of register */
            "lsrs	r4, r5, #2\n\t"             	// shift bit 6 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 5 of register */
            "lsrs	r4, r5, #1\n\t"             	// shift bit 5 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 4 of register */
            "lsrs	r4, r5, #0\n\t"             	// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 3 of register */
            "lsls	r4, r5, #1\n\t"             	// shift bit 3 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 2 of register */
            "lsls	r4, r5, #2\n\t"             	// shift bit 2 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 1 of register */
            "lsls	r4, r5, #3\n\t"             	// shift bit 1 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t" 	// drive clock HIGH

            /* bit 0 of register */
            "lsls	r4, r5, #4\n\t"             	// shift bit 0 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t" 	// drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "movs	r4, #16\n\t"                    // prepare the read bit: set bit 4
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* read bit: set ZDA */
            "ldr	r5, [%[port], #04]\n\t"         // current OTYPER
            "orrs	r5, r5, r4\n\t"                	// set PA4 to Open-Drain
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "str	r5, [%[port], #04]\n\t"         // change ZDA to open drain
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

    		/* now read */

            /* single-bit separator: drive ZDA low */
            "lsls	r4, r4, #16\n\t"                // shift over to reset ZDA
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive ZDA low
    		"movs	r5, #16\n\t"					// r5 written BSRR will set ZDA hi-Z
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* clock bit 7 of data */
            "str	r5, [%[port], #24]\n\t"			// ZDA now hi-Z
            "movs	%[data], #0\n\t"              	// %[data] will accumulate the incoming byte
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "nop\n\t"                               // delay to balance duty cycle
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 6 of data */
    		"nop\n\t"								// still waiting for data...
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 7 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 5 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 6 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 4 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 5 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 3 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 4 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 2 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 3 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 1 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 2 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 0 of data */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 1 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* single-bit separator: TODO who drives this? */
    		"lsls	%[data], %[data], #1\n\t"		// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 0 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	%[data], %[data], r4\n\t"		// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

    		/* shift the whole result right 4 bits */
    		"lsrs	%[data], %[data], #4\n\t"

            : [data]	"+l" (result)
            : [reg]     "r" (reg_bsrr_bits),
              [port]    "l" (0x50000000),
              [zcl_lo]  "l" (zcl_lo),
              [zcl_hi]  "l" (zcl_hi),
              [mask]    "l" (bsrr_mask)
            : "r4", "r5", "cc", "memory"
    );

    *otyper = zda_od;
    *bsrr = zcl_hi;

    /* Restore interrupts to previous state */
    __set_PRIMASK(primask);

	return result;
}
