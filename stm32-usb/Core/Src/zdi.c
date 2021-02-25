/*
 * zdi.c
 *
 *  Created on: Jan 10, 2021
 *      Author: bje
 */

#include <stdlib.h>
#include <stdint.h>

#include "stm32l0xx.h"
#include "zdi.h"

/**
 * Write a value to a ZDI register.
 *
 * Hopefully without melting down an expensive and hard to solder CPU.
 */
void zdi_write_block(uint8_t reg, uint8_t *data, uint16_t count)
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

    /* Turn off interrupts, if enabled */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* the cortex-m0+ only permits aligned memory access; prepare all the data bytes ahead of time */
    uint32_t *data_words = malloc(sizeof(uint32_t) * count);
    for (uint16_t i = 0; i < count; i++) {
    	data_words[i] = (~data[i]) << 16 | data[i];
    }

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
    		"movs	r4, 1\n\t"
    		"lsls   r4, 20\n\t"
            "str	r4, %[bsrr]\n\t"

            /* bit 6 of register */
            "lsrs	r4, r5, #2\n\t"             	// shift bit 6 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 5 of register */
            "lsrs	r4, r5, #1\n\t"             	// shift bit 5 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 4 of register */
            "lsrs	r4, r5, #0\n\t"             	// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 3 of register */
            "lsls	r4, r5, #1\n\t"             	// shift bit 3 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 2 of register */
            "lsls	r4, r5, #2\n\t"             	// shift bit 2 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 1 of register */
            "lsls	r4, r5, #3\n\t"             	// shift bit 1 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 0 of register */
            "lsls	r4, r5, #4\n\t"             	// shift bit 0 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "movs	r4, #128\n\t"                   // prepare the write bit: set bit 7
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* write bit: reset ZDA */
            "lsls	r4, r4, #13\n\t"                // shift bit 7 to bit 20
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* single-bit separator: keep low */
            "nop\n\t"                     			// delay to balance duty cycle
            "nop\n\t"                     			// delay to balance duty cycle
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "nop\n\t"                     			// delay to balance duty cycle
    		"1:\n\t"								// data write loop
            "ldr	r5, [%[data]]\n\t"              // load next byte to write
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 7 of data */
            "lsrs	r4, r5, #3\n\t"            		// shift bit 7 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 6 of data */
            "lsrs	r4, r5, #2\n\t"            		// shift bit 6 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 5 of data */
            "lsrs	r4, r5, #1\n\t"            		// shift bit 5 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 4 of data */
            "lsrs	r4, r5, #0\n\t"            		// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 3 of data */
            "lsls	r4, r5, #1\n\t"            		// shift bit 3 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 2 of data */
            "lsls	r4, r5, #2\n\t"            		// shift bit 2 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 1 of data */
            "lsls	r4, r5, #3\n\t"            		// shift bit 1 to bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
            "lsls	r4, r5, #4\n\t"            		// shift bit 0 to bit 4 (early, to free up a cycle)
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* bit 0 of data */
            "movs	r5, #16\n\t"                    // prepare the separator bit: set bit 4
            "ands	r4, %[mask]\n\t"                // mask BSRR bits
            "str	%[zcl_lo], %[bsrr]\n\t"       	// drive clock LOW
            "str	r4, %[bsrr]\n\t"              	// drive data
    		"mov	r4, %[count]\n\t"      			// decrement count
            "str	%[zcl_hi], %[bsrr]\n\t"       	// drive clock HIGH

            /* single-bit separator: drive low */
    		"subs   r4, r4, 1\n\t"					// decrement count - extra cycle
            "beq	1f\n\t"                         // wind up if zero - extra cycle
    		"mov   	%[count], r4\n\t"				// store the decremented count
            "lsls	r5, #16\n\t"                    // shift to reset bit 4
            "str	%[zcl_lo], %[bsrr]\n\t"         // drive clock LOW
            "str	r5, %[bsrr]\n\t"                // drive data
            "adds	%[data], 4\n\t"                 // shift data pointer up a word
    		"b		1b\n\t"							// loop back to load more data and drive clock high

    		"1:\n\t"
    		"str	%[zcl_lo], %[bsrr]\n\t"         // drive clock LOW
            "str	r5, %[bsrr]\n\t"                // drive data
            "nop\n\t"								// may as well _try_ to balance duty cycle
            "str	%[zcl_hi], %[bsrr]\n\t"         // drive clock HIGH

            :
    		: [data]	"l" (data_words),
			  [count]   "r" (count),
              [reg]     "r" (reg_bsrr_bits),
              [bsrr]    "m" (*bsrr),
              [zcl_lo]  "l" (zcl_lo),
              [zcl_hi]  "l" (zcl_hi),
			  [mask]    "l" (bsrr_mask)
            : "r4", "r5", "cc", "memory"
    );

    free(data_words);

    *otyper = zda_od;
    *bsrr = zcl_hi;

    /* Restore interrupts to previous state */
    __set_PRIMASK(primask);
}

void zdi_read_block(uint8_t reg, uint8_t *data, uint16_t count)
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

    /* Need to do all memory accesses 32-bit aligned */
    uint32_t *data_words = malloc(sizeof(uint32_t) * count);

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

    		/* prepare the bsrr mask for the register write */
    		"mov	r3, %[mask]\n\t"

            /* start signal */
    		"movs	r4, #16\n\t"					// ZDA set
    		"lsls	r4, r4, #16\n\t"				// shift to ZDA reset
            "str	r4, [%[port], #24]\n\t"			// *bsrr = ZDA reset
    		"mov	r5, %[reg]\n\t"					// r5 holds the value being sent

            /* bit 6 of register */
            "lsrs	r4, r5, #2\n\t"             	// shift bit 6 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 5 of register */
            "lsrs	r4, r5, #1\n\t"             	// shift bit 5 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 4 of register */
            "lsrs	r4, r5, #0\n\t"             	// shift bit 4 to bit 4 (aka, NOP)
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 3 of register */
            "lsls	r4, r5, #1\n\t"             	// shift bit 3 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 2 of register */
            "lsls	r4, r5, #2\n\t"             	// shift bit 2 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 1 of register */
            "lsls	r4, r5, #3\n\t"             	// shift bit 1 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive data
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t" 	// drive clock HIGH

            /* bit 0 of register */
            "lsls	r4, r5, #4\n\t"             	// shift bit 0 to bit 4
            "ands	r4, r3\n\t"  		            // mask BSRR bits
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

            /* single-bit separator: drive ZDA low */
            "lsls	r4, r4, #16\n\t"                // shift over to reset ZDA
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "str	r4, [%[port], #24]\n\t"         // drive ZDA low
    		"movs	r5, #16\n\t"					// r5 written BSRR will set ZDA hi-Z
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* clock bit 7 of data */
            "str	r5, [%[port], #24]\n\t"			// ZDA now hi-Z

    		// data read loop
    		"1:\n\t"
            "movs	r3, #0\n\t"              		// r3 will accumulate the incoming byte
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
            "nop\n\t"                               // delay to balance duty cycle
            "nop\n\t"                               // delay to balance duty cycle
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 6 of data */
    		"nop\n\t"								// still waiting for data...
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 7 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 5 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 6 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 4 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 5 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 3 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 4 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 2 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 3 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 1 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 2 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* bit 0 of data */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 1 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

            /* single-bit separator: TODO who drives this? */
    		"lsls	r3, r3, #1\n\t"					// shift result one to the left
    		"ldr	r4, [%[port], #16]\n\t"			// read from IDR (bit 0 finally here)
            "str	%[zcl_lo], [%[port], #24]\n\t"  // drive clock LOW
    		"ands	r4, r4, r5\n\t"					// mask to just PA4
    		"orrs	r3, r3, r4\n\t"					// write bit in to result
            "str	%[zcl_hi], [%[port], #24]\n\t"  // drive clock HIGH

    		/* housekeeping for a few cycles */
    		"strh	r3, [%[data]]\n\t"				// store the byte (left-shifted 4)
    		"adds 	%[data], %[data], #4\n\t"		// move to next word address
    		"mov 	r3, %[count]\n\t"				// get count
    		"subs	r3, r3, 1\n\t"					// decrement
    		"mov	%[count], r3\n\t"				// store count
    		"bne 	1b"								// if not zero, loop back for more data

            :
			: [count]   "r" (count),
              [data]    "l" (data_words),
			  [reg]     "r" (reg_bsrr_bits),
              [port]    "l" (0x50000000),
              [zcl_lo]  "l" (zcl_lo),
              [zcl_hi]  "l" (zcl_hi),
              [mask]    "r" (bsrr_mask)
            : "r3", "r4", "r5", "cc", "memory"
    );

    for (uint16_t i = 0; i < count; i++) {
    	data[i] = data_words[i] >> 4;
    }

    free(data_words);

    *otyper = zda_od;
    *bsrr = zcl_hi;

    /* Restore interrupts to previous state */
    __set_PRIMASK(primask);

}

void zdi_write(uint8_t reg, uint8_t data)
{
	zdi_write_block(reg, &data, 1);
}

uint8_t zdi_read(uint8_t reg)
{
	uint8_t result;
	zdi_read_block(reg, &result, 1);
	return result;
}
