;-------------------------------------------------------------------------
; Long Long Shift Left
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
	.xdef	__llshl

__llshl:
	push	iy

	push	hl
	pop	iy

	ld	b, c
	inc	b
	jr	test

shloop:
	sla	(iy+0)
	rl	(iy+1)
	rl	(iy+2)
	rl	(iy+3)
	rl	(iy+4)
	rl	(iy+5)
	rl	(iy+6)
	rl	(iy+7)

test:
	djnz	shloop

	ld	hl, (iy)
	ld	de, (iy+3)
	ld	c, (iy+6)
	ld	b, (iy+7)

	pop	iy
	ret

