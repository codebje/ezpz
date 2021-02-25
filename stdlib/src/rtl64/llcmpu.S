;-------------------------------------------------------------------------
; Long Long Comparison
; Input:
;	Operand1: 
;		  uHL : address of 64-bit value
;
;	Operand2: 
;		  uBC : address of 64-bit value
;
; Output:
;	Result:   F : S,V,Z flags from (HL) - (BC)
; Registers Used:
;	none
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.global	__llcmpu

__llcmpu:
	push	ix
	push	iy

	;; save a
	push	de
	ld	e, a

	ld	a, 1

	;; the subtract will determine C correctly
	;; need to AND the Z flag from each stage

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
	jr	z, next24
	ld	a, 0			; will need to reset ZF

next24:
	; then the next 24 bits
	ld	hl, (ix + 3)
	ld	bc, (iy + 3)
	sbc	hl, bc
	jr	z, final16
	ld	a, 0			; will need to reset ZF

final16:
	; then the final 16 bits
	ld	hl, (ix + 6)
	ld	bc, (iy + 6)
	sbc.s	hl, bc

	;; reset zf if a is zero
	or	a, a
	jr	nz, done

	push	af
	pop	hl		; L = flags
	res     6,l		; reset ZF, preserve other flags
	push	hl
	pop	af

done:
	;; restore bc, hl
	push	ix
	pop	hl
	push	iy
	pop	bc

	;; restore a
	ld	a, e
	pop	de

	pop	iy
	pop	ix
	ret
