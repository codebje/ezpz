	.text
	.assume adl=1

	in0	e, ($fb)

	ld	bc, $b649
	out0	($f5), b
	out0	($f5), c
	ld	a, $ff
	out0	($f9), a

	out0	($f5), b
	out0	($f5), c
	ld	a, $00
	out0	($fa), a

	ld	a, $01
	out0	($ff), a

erasing:
	in0	d, ($fb)
	bit	5, d
	jr	z, erasing

	push	de
	ld	de, $80000
	ld	hl, $00000
	ld	bc, $15800
	ldir
	pop	de

writing:
	in0	l, ($fb)
	bit	5, l
	jr	z, writing

	ld	bc, $b649
	out0	($f5), b
	out0	($f5), c
	ld	a, $ff
	out0	($fa), a

loop:	jr	loop
