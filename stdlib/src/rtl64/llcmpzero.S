;-------------------------------------------------------------------------
; Long Long Compare Zero
; Input:
;	Operand1: 
;                 HL : address of 64 bit value to be tested
; 
; Output:
;	Result:  f register
;
; Registers Used:
;	flags
;-------------------------------------------------------------------------
	.text
	.assume adl=1
	.xdef	__llcmpzero

__llcmpzero:
	push	iy
	push	hl
	pop	iy
	push	de
	ld	e, a
	xor	a

	cp	a, (iy)
	jr	nz, done
	cp	a, (iy+1)
	jr	nz, done
	cp	a, (iy+2)
	jr	nz, done
	cp	a, (iy+3)
	jr	nz, done
	cp	a, (iy+4)
	jr	nz, done
	cp	a, (iy+5)
	jr	nz, done
	cp	a, (iy+6)
	jr	nz, done
	cp	a, (iy+7)

done:
	ld	a, e
	pop	de
	pop	iy
	ret
