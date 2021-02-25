;-------------------------------------------------------------------------
; Long Long Shift Right Signed
; Input:
;	Operand1: 
;                 HL : address of 64 bit value to be shifted
; 
;	Operand2: 
;                 C   : number of bits to shift
;
; Output:
;	Result:   BCuDEuHL : 64 bit 
; Registers Used:
;	flags
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.xdef	__llshrs

__llshrs:
	push	iy

	push	hl
	pop	iy

	ld	b, c
	inc	b
	jr	test

shloop:
	sra	(iy+7)
	rr	(iy+6)
	rr	(iy+5)
	rr	(iy+4)
	rr	(iy+3)
	rr	(iy+2)
	rr	(iy+1)
	rr	(iy+0)

test:
	djnz	shloop

	ld	hl, (iy)
	ld	de, (iy+3)
	ld	c, (iy+6)
	ld	b, (iy+7)

	pop	iy
	ret

