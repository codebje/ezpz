	.assume	adl=1

	.global	__bpopcnt
__bpopcnt:
	push	bc
	ld	c,a		; c=(A|B|C|D|E|F|G|H)
	and	a,$aa		; a=(A|0|C|0|E|0|G|0)
	cpl			; a=(~A|1|~C|1|~E|1|~G|1)
	rrca			; a=(1|~A|1|~C|1|~E|1|~G), cf=1
	adc	a,c		; a=(A+B|C+D|E+F|G+H)
	ld	b,a		; b=(A+B|C+D|E+F|G+H)
	and	a,$33		; a=(00|C+D|00|G+H)
	ld	c,a		; c=(00|C+D|00|G+H)
	xor	a,b		; a=(A+B|00|E+F|00)
	rrca
	rrca			; a=(00|A+B|00|E+F)
	add	a,c		; a=(A+B+C+D|E+F+G+H)
	ld	c,a		; c=(A+B+C+D|E+F+G+H)
	rrca
	rrca
	rrca
	rrca			; a=(E+F+G+H|A+B+C+D)
	add	a,c		; a=(A+B+C+D+E+F+G+H|A+B+C+D+E+F+G+H)
	and	a,$f		; a=A+B+C+D+E+F+G+H
	pop	bc
	ret

