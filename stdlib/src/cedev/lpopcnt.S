	.assume	adl=1

	.global	__lpopcnt
__lpopcnt:
        push	hl
	push	de
	ld	a,l
    	call	__bpopcnt
	ld	l,a
	ld	a,h
	call	__bpopcnt
	ld	h,a
	ld	a,e
	call	__bpopcnt
	ld	e,a
	call	$21D5C
	call	__bpopcnt
	add	a,e
	add	a,l
	add	a,h
        pop	de
	pop	hl
	ret

	extern	__bpopcnt

