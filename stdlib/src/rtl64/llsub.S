;-------------------------------------------------------------------------
; Long Long Subtraction
; Input:
;	Operand1: 
;		  uHL : address of 64-bit value
;
;	Operand2: 
;		  uBC : address of 64-bit value
;
; Output:
;	Result:   BCuDEuHL : 64 bit = *HL - *BC
; Registers Used:
;	none
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.global	__llsub

__llsub:
	push	ix
	push	iy

	; set ix = operand1, iy = operand2
	push	hl
	pop	ix
	push	bc
	pop	iy

	; sub the lower 24 bits first
	ld	hl, (ix + 0)
	ld	bc, (iy + 0)
	or	a, a
	sbc	hl, bc
	push	hl

	; then the next 24 bits
	ld	hl, (ix + 3)
	ld	bc, (iy + 3)
	sbc	hl, bc
	push	hl

	; then the final 16 bits
	ld	hl, (ix + 6)
	ld	bc, (iy + 6)
	sbc.s	hl, bc
	push	hl

	pop	bc
	pop	de
	pop	hl

	pop	iy
	pop	ix
	ret
