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
	.global	__lland

__lland:
	push	af
	push	ix
	push	iy

	; set ix = operand1, iy = operand2
	push	hl
	pop	ix
	push	bc
	pop	iy

	ld	a, (ix)
	and	a, (iy)
	ld	l, a
	push	hl
	ld	a, (ix+1)
	and	a, (iy+1)
	ld	l, a
	ld	a, (ix+2)
	and	a, (iy+2)
	ld	h, a
	push	hl
	dec	sp
	pop	hl
	dec	sp
	dec	sp

	ld	a, (ix+3)
	and	a, (iy+3)
	ld	e, a
	push	de
	ld	a, (ix+4)
	and	a, (iy+4)
	ld	e, a
	ld	a, (ix+5)
	and	a, (iy+5)
	ld	d, a
	push	de
	dec	sp
	pop	de
	dec	sp
	dec	sp

	ld	a, (ix+6)
	and	a, (iy+6)
	ld	c, a
	ld	a, (ix+7)
	and	a, (iy+7)
	ld	b, a

	pop	iy
	pop	ix
	pop	af
	ret
