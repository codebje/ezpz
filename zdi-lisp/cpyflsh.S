	.text
	.assume adl=1

	in0	a, ($fb)

	;ld	bc, $b649
	;out0	($f5), b
	;out0	($f5), c
	;ld	a, $ff
	;out0	($f9), a	; set clock divisor to 255 for 50MHz cpu

	;out0	($f5), b
	;out0	($f5), c
	;ld	a, $00
	;out0	($fa), a	; unlock write protection

	;ld	a, $01
	;out0	($ff), a	; erase flash

;erasing:
	;in0	d, ($fb)
	;bit	5, d
	;jr	z, erasing

	;push	de
	ld	de, 0x80000
	ld	hl, 0x00000
	ld	bc, 0x16000
	ldir
	;pop	de

;writing:
	;in0	l, ($fb)
	;bit	5, l
	;jr	z, writing

	;ld	bc, $b649
	;out0	($f5), b
	;out0	($f5), c
	;ld	a, $ff
	;out0	($fa), a

loop:	jr	loop
