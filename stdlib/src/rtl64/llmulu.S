;-------------------------------------------------------------------------
; Long Long Multiplication Signed/Unsigned.
;	Note:	When multipying two n-bit values, the low-order n-bits
;		of the product are the same, whether or not the values
;		are considered signed or unsigned.
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
	.global	__llmulu

;-------------------------------------------------------------------------
; A 64-bit number is two 32-bit numbers: x(64) = x.hi(32) << 32 + x.lo(32)
; Multiplying two 64-bit numbers x(64) * y(64) is a factor product:
;    x * y = (x.hi << 32) * (y.hi << 32)
;          + (x.hi << 32) * y.lo
;          + x.lo * (y.hi << 32)
;          + x.lo * y.lo
; Discard the first term (x.hi * y.hi << 64), all result bits are truncated
; The second and third terms are 32-bit multiplications truncated to 32-bits
; which can be computed with __lmulu
; The fourth term is a 64-bit result from a 32-bit multiplication, and again
; factorisation helps:
;    x.lo(32) = x.lh(16) << 16 + x.ll(16)
;    x.lo * y.lo = (x.lh * y.lh) << 32
;                + (x.lh * y.ll) << 16
;                + (x.ll * y.lh) << 16
;                + (x.ll * y.ll)
;
; Factorisation can be repeated down to 8-bit values multipled into 16-bit
; results, which the eZ80 can perform - at least for unsigned numbers.

; frame storage:
;	ix-3	hl, &operand1
;	ix-6	bc, &operand2
;	ix-14	64-bit result space
;	ix-22	64-bit intermediate 1
;	ix-30	64-bit intermediate 2
__llmulu:
	ld	hl, -30
	call	__frameset

	; retain the input pointers
	ld	(ix - 6), bc
	ld	(ix - 3), hl

	; start with + (x.hi * y.lo + x.lo * y.hi) << 32
	ld	iy, (ix - 3)
	ld	hl, (iy + 4)		; EuHL = x.hi
	ld	e, (iy + 7)
	ld	iy, (ix - 6)
	ld	bc, (iy)		; AuBC = y.lo
	ld	a, (iy + 3)
	call	__lmulu
	ld	(ix - 10), hl		; store the result here for now
	ld	(ix - 7), e
	ld	iy, (ix - 3)
	ld	hl, (iy)		; EuHL = x.lo
	ld	e, (iy + 3)
	ld	iy, (ix - 6)
	ld	bc, (iy + 4)		; AuBC = y.hi
	ld	a, (iy + 7)
	call	__lmulu
	ld	bc, (ix - 10)		; retrieve the first part
	ld	a, (ix - 7)
	call	__ladd
	ld	(ix - 10), hl		; and store in the upper 32 bits of result
	ld	(ix - 7), e
	ld	bc, 0
	ld	(ix - 14), bc		; zero out the lower 32 bits
	ld	(ix - 11), b

	; compute x.lo * y.lo - first, x.lo.hi * y.lo.hi
	xor	a
	ld	e, a
	sbc	hl, hl
	ld	iy, (ix - 3)
	ld	l, (iy + 2)
	ld	h, (iy + 3)
	ld	iy, (ix - 6)
	ld	c, (iy + 2)
	ld	b, (iy + 3)
	call	__lmulu
	ld	(ix - 18), hl		; store in upper 32 bits of intermediate
	ld	(ix - 15), e

	; then, x.lo.lo * y.lo.lo
	xor	a
	ld	e, a
	sbc	hl, hl
	ld	bc, 0
	ld	iy, (ix - 3)
	ld	l, (iy + 0)
	ld	h, (iy + 1)
	ld	iy, (ix - 6)
	ld	c, (iy + 0)
	ld	b, (iy + 1)
	call	__lmulu
	ld	(ix - 22), hl		; store in lower 32 bits of intermediate
	ld	(ix - 19), e

	; then x.lo.hi * y.lo.lo + xl.lo.lo * y.lo.hi
	xor	a
	ld	e, a
	sbc	hl, hl
	ld	bc, 0
	ld	(ix - 30), hl		; zero out intermediate 2's upper and lower 24 bits
	ld	(ix - 25), hl		; so this sum can be stored in the middle
	ld	iy, (ix - 3)
	ld	l, (iy + 2)
	ld	h, (iy + 3)
	ld	iy, (ix - 6)
	ld	c, (iy + 0)
	ld	b, (iy + 1)
	call	__lmulu
	ld	(ix - 28), hl		; store result shifted left by 16
	ld	(ix - 25), e
	xor	a
	ld	e, a
	sbc	hl, hl
	ld	bc, 0
	ld	iy, (ix - 3)
	ld	l, (iy + 0)
	ld	h, (iy + 1)
	ld	iy, (ix - 6)
	ld	c, (iy + 2)
	ld	b, (iy + 3)
	call	__lmulu
	ld	bc, (ix - 28)
	ld	a, (ix - 25)
	call	__ladd
	ld	(ix - 28), hl		; store result shifted left by 16
	ld	(ix - 25), e

	; add two intermediates
	lea	bc, ix + 30
	lea	hl, ix + 22
	call	__lladd
	ld	(ix - 22), hl		; store sum into intermediate 1
	ld	(ix - 19), de
	ld	(ix - 16), b
	ld	(ix - 15), c

	; add two parts into final result
	lea	bc, ix + 22
	lea	hl, ix + 14
	call	__lladd

	ld	sp, ix
	pop	ix
	ret
