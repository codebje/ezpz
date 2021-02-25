;-------------------------------------------------------------------------
; Long Long Divide Signed
; Input:
;	Operand1: 
;                 BC : address of 64 bit value of dividend
; 
;	Operand2: 
;                 HL : address of 64 bit value of divisor
;
; Output:
;	Result:   BCuDEuHL : 64 bit 
; Registers Used:
;	None
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.xdef	__lldivs

; -a/-b = a/b; -a/b = -(a/b); a/-b = -(a/b)
; so negate anything negative, and count how many times it was done
__lldivs:
	push	ix
	push	iy
	push	af

	push	hl
	pop	ix
	push	bc
	pop	iy

	; store the most significant bytes in de
	ld	d, (ix+7)
	ld	e, (iy+7)
	push	de

	bit	7, d
	jr	z, posdivisor

	call	neg		; in-place negate (ix)

	pop	de
	push	de

posdivisor:
	bit	7, e
	jr	z, posdividend

	lea	iy, ix+0
	call	neg

posdividend:
	call	__lldivu	; unsigned divide (hl) by (bc)

	; check the origin
	pop	ix
	ld	a, ixh
	xor	a, ixl
	jp	p, samesign

	; need to negate BCuDEuHL: just pop them on the stack to do it

	push	bc
	push	de
	push	hl
	ld	ix,0
	add	ix,sp
	call	neg
	pop	hl
	pop	de
	pop	bc

samesign:
	pop	af
	pop	iy
	pop	ix
	ret

neg:	push	hl

	or	a, a
	sbc	hl, hl
	ld	de, (ix+0)
	sbc	hl, de
	ld	(ix+0), hl
	ld	de, (ix+3)
	ld	hl, 0
	sbc	hl, de
	ld	(ix+3), hl
	ld	e, (ix+6)
	ld	d, (ix+7)
	ld	hl, 0
	sbc.s	hl, de
	ld	(ix+6), l
	ld	(ix+7), h

	pop	hl
	ret
