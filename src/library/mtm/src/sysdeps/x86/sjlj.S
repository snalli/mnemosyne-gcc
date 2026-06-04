// Should we replace with arch_X*6.S from tinySTM ?
	.hidden	mtm_pwbetl_beginTransaction_internal
	.type	_ITM_beginTransaction,@function
	.align	2,0x90
	.global	_ITM_beginTransaction
_ITM_beginTransaction:
	mov	24(%rdi),%rax
	mov	%rbx,8(%rax)
	mov	%rbp,16(%rax)
	mov	%r12,24(%rax)
	mov	%r13,32(%rax)
	mov	%r14,40(%rax)
	mov	%r15,48(%rax)
	lea	8(%rsp),%r10
	mov	%r10,0(%rax)
	mov	(%rsp),%r10
	mov	%r10,56(%rax)
	stmxcsr 64(%rax)
	fnstcw  72(%rax)
	fclex
	jmp	mtm_pwbetl_beginTransaction_internal	# Vector to the target routine.


	.type	mtm_longjmp,@function
	.align	2,0x90
	.global	mtm_longjmp
mtm_longjmp:
	mov	8(%rdi),%rbx
	mov	16(%rdi),%rbp
	mov	24(%rdi),%r12
	mov	32(%rdi),%r13
	mov	40(%rdi),%r14
	mov	48(%rdi),%r15
	mov	0(%rdi),%rsp
	ldmxcsr	64(%rdi)
	fnclex
	fldcw	72(%rdi)
	cld
	emms
	mov	%rsi,%rax
	mov	56(%rdi),%rsi
	jmp	*%rsi
