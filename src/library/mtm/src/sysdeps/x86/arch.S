/*
 * Architecture-specific setjmp/longjmp glue for the GCC transactional-memory
 * ABI (-fgnu-tm).
 *
 * GCC emits a *single-argument* call:  ret = _ITM_beginTransaction(flags);
 * and expects this routine to behave like setjmp: it must capture the
 * *caller's* CPU context (stack pointer, return address, callee-saved
 * registers) so that an abort deep inside the transaction body can
 * _ITM_siglongjmp() back to the call site, re-entering it with a restart
 * action code in the return-value register.
 *
 * This is irreducibly architecture specific -- it cannot be expressed in
 * portable C, because a C function that simply returns can never be the
 * target of a longjmp once the transaction body (which runs in the caller's
 * frame, many frames above) is executing.  GCC's own libitm provides the
 * same routine in per-architecture assembly for exactly this reason.
 *
 * Supported: x86-64, AArch64.  The file is assembled with the C preprocessor
 * (.S), so the correct block is selected automatically from the compiler's
 * predefined target macros -- no platform/arch flags required.
 *
 * jmpbuf layout (offsets from the pointer passed to MTM_begin_transaction):
 *      [0]  caller stack pointer  (read as the SP by mtm_local_rollback)
 *      [8]  resume address        (jumped to by _ITM_siglongjmp)
 *      ...  callee-saved registers
 */

#ifndef __linux__
/* Linux specific; Windows uses a different calling convention. */
# error This file is linux specific.
#endif

	.text

#if defined(__x86_64__)

	.align	2,0x90
	.globl	_ITM_beginTransaction
	.type	_ITM_beginTransaction, @function
	.hidden	MTM_begin_transaction
_ITM_beginTransaction:
	.cfi_startproc
	/* rdi=flags. Save rbx, rsp, rbp, r12-r15. */
	leaq	8(%rsp), %rax		/* caller's rsp (above return addr) */
	subq	$56, %rsp
	.cfi_def_cfa_offset 64
	movq	%rax,  (%rsp)		/* [0]  sp                         */
	movq	%rbx, 8(%rsp)		/* [8]  rbx                        */
	movq	%rbp, 16(%rsp)		/* [16] rbp                        */
	movq	%r12, 24(%rsp)		/* [24] r12                        */
	movq	%r13, 32(%rsp)		/* [32] r13                        */
	movq	%r14, 40(%rsp)		/* [40] r14                        */
	movq	%r15, 48(%rsp)		/* [48] r15                        */
	/* [56] (= rsp at entry) already holds the user return address
	 * pushed by the call instruction; it serves as abendPC.          */
	movq	%rsp, %rsi		/* second arg = &jmpbuf            */
	call	MTM_begin_transaction
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
	.size	_ITM_beginTransaction, .-_ITM_beginTransaction

	.align	2,0x90
	.globl	_ITM_siglongjmp
	.type	_ITM_siglongjmp, @function
	.hidden	_ITM_siglongjmp
_ITM_siglongjmp:			/* (rdi=jmpbuf, esi=code) */
	.cfi_startproc
	movq	  (%rdi), %rcx		/* sp        */
	movq	 8(%rdi), %rbx
	movq	16(%rdi), %rbp
	movq	24(%rdi), %r12
	movq	32(%rdi), %r13
	movq	40(%rdi), %r14
	movq	48(%rdi), %r15
	movl	%esi, %eax		/* return value = action code */
	.cfi_def_cfa %rdi, 0
	.cfi_offset %rip, 56
	.cfi_register %rsp, %rcx
	movq	%rcx, %rsp
	jmp	*56(%rdi)		/* resume at the user call site */
	.cfi_endproc
	.size	_ITM_siglongjmp, .-_ITM_siglongjmp

	.align	2,0x90
	.globl	mtm_longjmp
	.type	mtm_longjmp, @function
mtm_longjmp:				/* (rdi=jmpbuf, rsi=val) -- unused legacy */
	movq	%rsi, %rax
	movq	 8(%rdi), %rbx
	movq	16(%rdi), %rbp
	movq	24(%rdi), %r12
	movq	32(%rdi), %r13
	movq	40(%rdi), %r14
	movq	48(%rdi), %r15
	movq	56(%rdi), %rsi
	jmp	*%rsi
	.size	mtm_longjmp, .-mtm_longjmp

#elif defined(__aarch64__)

	.align	2
	.globl	_ITM_beginTransaction
	.type	_ITM_beginTransaction, %function
_ITM_beginTransaction:
	.cfi_startproc
	/* x0 = flags.  Build the jmpbuf inside our own frame, hand &jmpbuf
	 * to MTM_begin_transaction in x1, then return normally.  336 bytes
	 * keeps the whole sizeof(jmp_buf) copy that MTM_begin_transaction
	 * performs within this frame. */
	stp	x29, x30, [sp, #-336]!	/* save user fp/lr at [sp,#0]/[sp,#8] */
	.cfi_def_cfa_offset 336
	mov	x29, sp
	add	x9,  sp, #336		/* caller's sp                        */
	ldr	x10, [sp, #8]		/* user resume address (orig x30)     */
	ldr	x11, [sp, #0]		/* user frame pointer  (orig x29)     */
	str	x9,  [sp, #16]		/* jmpbuf[0]   sp                     */
	str	x10, [sp, #24]		/* jmpbuf[8]   resume addr            */
	str	x11, [sp, #32]		/* jmpbuf[16]  fp                     */
	stp	x19, x20, [sp, #40]	/* jmpbuf[24]                        */
	stp	x21, x22, [sp, #56]
	stp	x23, x24, [sp, #72]
	stp	x25, x26, [sp, #88]
	stp	x27, x28, [sp, #104]
	stp	d8,  d9,  [sp, #120]
	stp	d10, d11, [sp, #136]
	stp	d12, d13, [sp, #152]
	stp	d14, d15, [sp, #168]
	add	x1,  sp, #16		/* second arg = &jmpbuf              */
	bl	MTM_begin_transaction
	ldp	x29, x30, [sp], #336
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
	.size	_ITM_beginTransaction, .-_ITM_beginTransaction

	.align	2
	.globl	_ITM_siglongjmp
	.type	_ITM_siglongjmp, %function
_ITM_siglongjmp:			/* (x0=jmpbuf, w1=code) */
	.cfi_startproc
	ldr	x9,  [x0, #0]		/* sp          */
	ldr	x30, [x0, #8]		/* resume addr */
	ldr	x29, [x0, #16]		/* fp          */
	ldp	x19, x20, [x0, #24]
	ldp	x21, x22, [x0, #40]
	ldp	x23, x24, [x0, #56]
	ldp	x25, x26, [x0, #72]
	ldp	x27, x28, [x0, #88]
	ldp	d8,  d9,  [x0, #104]
	ldp	d10, d11, [x0, #120]
	ldp	d12, d13, [x0, #136]
	ldp	d14, d15, [x0, #152]
	mov	sp,  x9
	mov	w0,  w1			/* return value = action code */
	ret				/* br x30 -> resume at user call site */
	.cfi_endproc
	.size	_ITM_siglongjmp, .-_ITM_siglongjmp

	.align	2
	.globl	mtm_longjmp
	.type	mtm_longjmp, %function
mtm_longjmp:				/* (x0=jmpbuf, w1=val) -- unused legacy */
	.cfi_startproc
	ldr	x9,  [x0, #0]
	ldr	x30, [x0, #8]
	ldr	x29, [x0, #16]
	ldp	x19, x20, [x0, #24]
	ldp	x21, x22, [x0, #40]
	ldp	x23, x24, [x0, #56]
	ldp	x25, x26, [x0, #72]
	ldp	x27, x28, [x0, #88]
	ldp	d8,  d9,  [x0, #104]
	ldp	d10, d11, [x0, #120]
	ldp	d12, d13, [x0, #136]
	ldp	d14, d15, [x0, #152]
	mov	sp,  x9
	mov	w0,  w1
	ret
	.cfi_endproc
	.size	mtm_longjmp, .-mtm_longjmp

#else
# error "Unsupported architecture: _ITM_beginTransaction sjlj glue required"
#endif

#ifdef __linux__
.section .note.GNU-stack, "", @progbits
#endif
