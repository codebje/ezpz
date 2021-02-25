;-------------------------------------------------------------------------
; Long Long Divide Unsigned
; Input:
;	Operand1: 
;                 BC : address of 64 bit value of divisor
; 
;	Operand2: 
;                 HL : address of 64 bit value of dividend
;
; Output:
;	Result:   BCuDEuHL : 64 bit (HL)/(BC)
; Registers Used:
;	None
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.xdef	__lldivu

	; set remainder = 0
	; 64 times:
	; shift dividend left one
	; shift remainder left one, bringing in carry from dividend
	; subtract divisor from remainder
	; if < 0, undo subtract
	; else set bit 0 in dividend

__lldivu:
	push	iy
	push	ix
	ld	ix, 0
	add	ix, sp

	push	hl
	pop	iy
	ld	hl, (iy+6)
	push	hl
	ld	hl, (iy+3)
	push	hl
	ld	hl, (iy+0)
	push	hl		; dividend is at (ix-9)

	or	a, a
	sbc	hl, hl
	push	hl
	push	hl
	push	hl		; remainder is at (ix-18)

	push	bc		; just store the pointer to the divisor at (ix-21)

	ld	b, 64		; how many times around will we go?

L_word:
	sla	(ix-9)		; shift the dividend left one
	rl	(ix-8)
	rl	(ix-7)
	rl	(ix-6)
	rl	(ix-5)
	rl	(ix-4)
	rl	(ix-3)
	rl	(ix-2)
	rl	(ix-18)		; and continue on into the remainder
	rl	(ix-17)
	rl	(ix-16)
	rl	(ix-15)
	rl	(ix-14)
	rl	(ix-13)
	rl	(ix-12)
	rl	(ix-11)		; carry flag always RESET after shifting

	push	bc

	lea	hl, ix-18
	ld	bc, (ix-21)
	call	__llsub		; BCuDEuHL = remainder - divisor

	jr	c, L_next	; if remainder < divisor, forget the result and loop

	ld	(ix-18), hl	; store the updated remainder
	ld	(ix-15), de
	ld	(ix-12), c
	ld	(ix-11), b

	inc	(ix-9)		; set bit 0 of the dividend

L_next:
	pop	bc
	djnz	L_word

	ld	hl, (ix-9)
	ld	de, (ix-6)
	ld	c, (ix-3)
	ld	b, (ix-2)

	ld	sp, ix
	pop	ix
	pop	iy
	ret
