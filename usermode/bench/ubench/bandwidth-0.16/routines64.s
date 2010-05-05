
; ============================================================================
;  bandwidth 0.16, a benchmark to estimate memory transfer bandwidth.
;  Copyright (C) 2005,2007-2009 by Zack T Smith.
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;
;  The author may be reached at fbui@comcast.net.
; =============================================================================

cpu	x64
bits	64

global	Reader
global	ReaderSSE2
global	Writer
global	WriterSSE2
global	my_bzero
global	my_bzeroSSE2

; Note to self:
; Unix ABI says integer param are put in these registers in this order:
;	rdi, rsi, rdx, rcx, r8, r9

	section .text

;------------------------------------------------------------------------------
; Name:		Reader
; Purpose:	Reads 64-bit values sequentially from an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = loops
; 		rdx = length in bytes
;------------------------------------------------------------------------------
Reader:
	mov	r11, rdi
	add	r11, rdx	; r11 points to end or area.

r_outer:
	mov	r10, rdi

r_inner:
	mov	rax, [r10]
	mov	rax, [8+r10]
	mov	rax, [16+r10]
	mov	rax, [24+r10]
	mov	rax, [32+r10]
	mov	rax, [40+r10]
	mov	rax, [48+r10]
	mov	rax, [56+r10]
	mov	rax, [64+r10]
	mov	rax, [72+r10]
	mov	rax, [80+r10]
	mov	rax, [88+r10]
	mov	rax, [96+r10]
	mov	rax, [104+r10]
	mov	rax, [112+r10]
	mov	rax, [120+r10]
	mov	rax, [128+r10]
	mov	rax, [136+r10]
	mov	rax, [144+r10]
	mov	rax, [152+r10]
	mov	rax, [160+r10]
	mov	rax, [168+r10]
	mov	rax, [176+r10]
	mov	rax, [184+r10]
	mov	rax, [192+r10]
	mov	rax, [200+r10]
	mov	rax, [208+r10]
	mov	rax, [216+r10]
	mov	rax, [224+r10]
	mov	rax, [232+r10]
	mov	rax, [240+r10]
	mov	rax, [248+r10]

	add	r10, 256
	cmp	r10, r11
	jb	r_inner

	dec	rsi
	jnz	r_outer
	ret

;------------------------------------------------------------------------------
; Name:		ReaderSSE2
; Purpose:	Reads 128-bit values sequentially from an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = loops
; 		rdx = length in bytes
;------------------------------------------------------------------------------
ReaderSSE2:
	mov	r11, rdi
	add	r11, rdx	; r11 points to end or area.

r_outer2:
	mov	r10, rdi

r_inner2:
;	movdqa	xmm0, [r10]
;	movdqa	xmm0, [16+r10]
;	movdqa	xmm0, [32+r10]
;	movdqa	xmm0, [48+r10]
;	movdqa	xmm0, [64+r10]
;	movdqa	xmm0, [80+r10]
;	movdqa	xmm0, [96+r10]
;	movdqa	xmm0, [112+r10]
;
;	movdqa	xmm0, [128+r10]
;	movdqa	xmm0, [144+r10]
;	movdqa	xmm0, [160+r10]
;	movdqa	xmm0, [176+r10]
;	movdqa	xmm0, [192+r10]
;	movdqa	xmm0, [208+r10]
;	movdqa	xmm0, [224+r10]
;	movdqa	xmm0, [240+r10]

	movdqa	xmm0, [r10]
	movdqa	xmm0, [16+r10]
	movdqa	xmm0, [32+r10]
	movdqa	xmm0, [48+r10]
	movdqa	xmm0, [64+r10]
	movdqa	xmm0, [80+r10]
	movdqa	xmm0, [96+r10]
	movdqa	xmm0, [112+r10]

	movdqa	xmm0, [128+r10]
	movdqa	xmm0, [144+r10]
	movdqa	xmm0, [160+r10]
	movdqa	xmm0, [176+r10]
	movdqa	xmm0, [192+r10]
	movdqa	xmm0, [208+r10]
	movdqa	xmm0, [224+r10]
	movdqa	xmm0, [240+r10]

	add	r10, 256
	cmp	r10, r11
	jb	r_inner2

	dec	rsi
	jnz	r_outer2
	ret


;------------------------------------------------------------------------------
; Name:		Writer
; Purpose:	Writes 64-bit value sequentially to an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = loops
; 		rdx = length in bytes
; 		rcx = quad to write
;------------------------------------------------------------------------------
Writer:

	mov	r11, rdi
	add	r11, rdx	; r11 points to end of area.
	mov	rax, rcx

w_outer:
	mov	r10, rdi

w_inner:
	mov	[r10], rcx
	mov	[8+r10], rcx
	mov	[16+r10], rcx
	mov	[24+r10], rcx
	mov	[32+r10], rcx
	mov	[40+r10], rcx
	mov	[48+r10], rcx
	mov	[56+r10], rcx
	mov	[64+r10], rcx
	mov	[72+r10], rcx
	mov	[80+r10], rcx
	mov	[88+r10], rcx
	mov	[96+r10], rcx
	mov	[104+r10], rcx
	mov	[112+r10], rcx
	mov	[120+r10], rcx
	mov	[128+r10], rcx
	mov	[136+r10], rcx
	mov	[144+r10], rcx
	mov	[152+r10], rcx
	mov	[160+r10], rcx
	mov	[168+r10], rcx
	mov	[176+r10], rcx
	mov	[184+r10], rcx
	mov	[192+r10], rcx
	mov	[200+r10], rcx
	mov	[208+r10], rcx
	mov	[216+r10], rcx
	mov	[224+r10], rcx
	mov	[232+r10], rcx
	mov	[240+r10], rcx
	mov	[248+r10], rcx

	add	r10, 256
	cmp	r10, r11
	jb	w_inner

	dec	rsi
	jnz	w_outer
	ret

;------------------------------------------------------------------------------
; Name:		WriterSSE2
; Purpose:	Writes 128-bit value sequentially to an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = loops
; 		rdx = length in bytes
; 		rcx = quad to write
;------------------------------------------------------------------------------
WriterSSE2:
	mov	r11, rdi
	add	r11, rdx	; r11 points to end of area.
	;movq	xmm0, rcx

w_outer2:
	mov	r10, rdi

w_inner2:
	movntdq	[r10], xmm0
	movntdq	[16+r10], xmm0
	movntdq	[32+r10], xmm0
	movntdq	[48+r10], xmm0
	movntdq	[64+r10], xmm0
	movntdq	[80+r10], xmm0
	movntdq	[96+r10], xmm0
	movntdq	[112+r10], xmm0

	;movntdq	[128+r10], xmm0
	;movntdq	[144+r10], xmm0
	;movntdq	[160+r10], xmm0
	;movntdq	[176+r10], xmm0
	;movntdq	[192+r10], xmm0
	;movntdq	[208+r10], xmm0
	;movntdq	[224+r10], xmm0
	;movntdq	[240+r10], xmm0

	add	r10, 128
	cmp	r10, r11
	jb	w_inner2

	dec	rsi
	jnz	w_outer2
	ret

;------------------------------------------------------------------------------
; Name:		my_bzero
; Purpose:	Writes 64-bit values sequentially to an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = length in bytes
;------------------------------------------------------------------------------
my_bzero:
	mov	rcx, rsi
	shr	rcx, 3
	xor	rax, rax
	cld
	rep stosq
	ret

;------------------------------------------------------------------------------
; Name:		my_bzeroSSE2
; Purpose:	Writes 128-bit zero values sequentially to an area of memory.
; Params:	rdi = ptr to memory area
; 		rsi = length in bytes
;------------------------------------------------------------------------------
my_bzeroSSE2:
	mov	r11, rdi
	add	r11, rsi	; r11 points to end or area.

	xor	rax, rax
	movq	xmm0, rax

	mov	r10, rdi

r_inner3:
	movntdq	[r10], xmm0
	movntdq	[16+r10], xmm0
	movntdq	[32+r10], xmm0
	movntdq	[48+r10], xmm0
	movntdq	[64+r10], xmm0
	movntdq	[80+r10], xmm0
	movntdq	[96+r10], xmm0
	movntdq	[112+r10], xmm0

	add	r10, 128
	cmp	r10, r11
	jb	r_inner3

	ret

;------------------------------------------------------------------------------
; Name:		has_sse2 (unfinished)
; 
has_sse2:
	mov	eax, 0
	cpuid
	ret

