
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

bits	32

global	Reader
global	Writer
global	my_bzero

	section .text

;------------------------------------------------------------------------------
; Name:		Reader
; Purpose:	Reads 32-bit values sequentially from an area of memory.
; Params:	
;		[esp+4]	= ptr to memory area
; 		[esp+8] = loops
; 		[esp+12] = length in bytes
;------------------------------------------------------------------------------
Reader:
	mov	ecx, [esp+8]	; loops to do.

	mov	esi, edx
	add	esi, [esp+12]

r_outer:
	mov	edx, [esp+4]	; ptr to memory chunk.

r_inner:
	mov	eax, [edx]
	mov	eax, [4+edx]
	mov	eax, [8+edx]
	mov	eax, [12+edx]
	mov	eax, [16+edx]
	mov	eax, [20+edx]
	mov	eax, [24+edx]
	mov	eax, [28+edx]
	mov	eax, [32+edx]
	mov	eax, [36+edx]
	mov	eax, [40+edx]
	mov	eax, [44+edx]
	mov	eax, [48+edx]
	mov	eax, [52+edx]
	mov	eax, [56+edx]
	mov	eax, [60+edx]
	mov	eax, [64+edx]
	mov	eax, [68+edx]
	mov	eax, [72+edx]
	mov	eax, [76+edx]
	mov	eax, [80+edx]
	mov	eax, [84+edx]
	mov	eax, [88+edx]
	mov	eax, [92+edx]
	mov	eax, [96+edx]
	mov	eax, [100+edx]
	mov	eax, [104+edx]
	mov	eax, [108+edx]
	mov	eax, [112+edx]
	mov	eax, [116+edx]
	mov	eax, [120+edx]
	mov	eax, [124+edx]

	add	edx, 128
	cmp	edx, esi
	jb	r_inner

	dec	ecx
	jnz	r_outer
	ret


;------------------------------------------------------------------------------
; Name:		Writer
; Purpose:	Writes 32-bit value sequentially to an area of memory.
; Params:	
;		[esp+4]	= ptr to memory area
; 		[esp+8] = loops
; 		[esp+12] = length in bytes
; 		[esp+16] = long to write
;------------------------------------------------------------------------------
Writer:
	mov	eax, [esp+16]
	mov	ecx, [esp+8]

	mov	esi, edx
	add	esi, [esp+12]

w_outer:
	mov	edx, [esp+4]

w_inner:
	mov	[edx], eax
	mov	[4+edx], eax
	mov	[8+edx], eax
	mov	[12+edx], eax
	mov	[16+edx], eax
	mov	[20+edx], eax
	mov	[24+edx], eax
	mov	[28+edx], eax
	mov	[32+edx], eax
	mov	[36+edx], eax
	mov	[40+edx], eax
	mov	[44+edx], eax
	mov	[48+edx], eax
	mov	[52+edx], eax
	mov	[56+edx], eax
	mov	[60+edx], eax
	mov	[64+edx], eax
	mov	[68+edx], eax
	mov	[72+edx], eax
	mov	[76+edx], eax
	mov	[80+edx], eax
	mov	[84+edx], eax
	mov	[88+edx], eax
	mov	[92+edx], eax
	mov	[96+edx], eax
	mov	[100+edx], eax
	mov	[104+edx], eax
	mov	[108+edx], eax
	mov	[112+edx], eax
	mov	[116+edx], eax
	mov	[120+edx], eax
	mov	[124+edx], eax

	add	edx, 128
	cmp	edx, esi
	jb	w_inner

	dec	ecx
	jnz	w_outer
	ret


