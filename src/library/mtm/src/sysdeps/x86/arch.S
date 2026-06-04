#ifndef __linux__
/* Linux specific, Windows uses a different calling convention. */
/* TODO probably solaris is fine with it. */
# error This file is linux specific.
#endif

	.text

#if defined(__x86_64__)
	.align 2,0x90
	.globl	_ITM_beginTransaction
	.type	_ITM_beginTransaction, @function
	.hidden MTM_begin_transaction
_ITM_beginTransaction:
	.cfi_startproc
/* Paramters (in order) is in rdi, rsi, rdx, rcx, r8, r9 */
/* Temporary registers is r10, r11 (not saved) */
/* To be saved are rbx, rsp, rbp, r12, r13, r14, r15 */
	leaq	8(%rsp), %rax    /* Save stack pointer */
	subq	$56, %rsp
	.cfi_def_cfa_offset 64
	movq	%rax, (%rsp)
	movq	%rbx, 8(%rsp)
	movq	%rbp, 16(%rsp)
	movq	%r12, 24(%rsp)
	movq	%r13, 32(%rsp)
	movq	%r14, 40(%rsp)
	movq	%r15, 48(%rsp)
	movq	%rsp, %rsi
	call	MTM_begin_transaction 
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
	.size	_ITM_beginTransaction, .-_ITM_beginTransaction

	.align 2,0x90
	.globl	_ITM_siglongjmp
	.type	_ITM_siglongjmp, @function
	.hidden	_ITM_siglongjmp

_ITM_siglongjmp:
	.cfi_startproc
	movq	(%rdi), %rcx
	movq	8(%rdi), %rbx
	movq	16(%rdi), %rbp
	movq	24(%rdi), %r12
	movq	32(%rdi), %r13
	movq	40(%rdi), %r14
	movq	48(%rdi), %r15
	movl	%edi, %eax
	.cfi_def_cfa %rdi, 0
	.cfi_offset %rip, 56
	.cfi_register %rsp, %rcx
	movq	%rcx, %rsp
	jmp	*56(%rdi)
	.cfi_endproc
	.size	_ITM_siglongjmp, .-_ITM_siglongjmp

        .type   mtm_longjmp,@function
        .align  2,0x90
        .global mtm_longjmp

mtm_longjmp:
        mov     %rsi,%rax
        // mov     0(%rdi),%rsp
        mov     8(%rdi),%rbx
        mov     16(%rdi),%rbp
        mov     24(%rdi),%r12
        mov     32(%rdi),%r13
        mov     40(%rdi),%r14
        mov     48(%rdi),%r15
        mov     56(%rdi),%rsi
        jmp     *%rsi

# else
# error Unsupported architecture
#endif

#ifdef __linux__
.section .note.GNU-stack, "", @progbits
#endif
