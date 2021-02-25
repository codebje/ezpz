;-------------------------------------------------------------------------
; Long Long Addition
; Input:
;	Operand1: 
;		  uHL : address of 64-bit value
;
;	Operand2: 
;		  uBC : address of 64-bit value
;
; Output:
;	Result:   BCuDEuHL : 64 bit
; Registers Used:
;	none
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.global	__lladd

__lladd:
	push	ix
	push	iy

	; set ix = operand1, iy = operand2
	push	hl
	pop	ix
	push	bc
	pop	iy

	; add the lower 24 bits first
	ld	hl, (ix + 0)
	ld	bc, (iy + 0)
	add	hl, bc
	push	hl

	; then the next 24 bits
	ld	hl, (ix + 3)
	ld	bc, (iy + 3)
	adc	hl, bc
	push	hl

	; then the final 16 bits
	ld	hl, (ix + 6)
	ld	bc, (iy + 6)
	adc.s	hl, bc
	push	hl

	pop	bc
	pop	de
	pop	hl

	pop	iy
	pop	ix
	ret
