	.file	"badcnt.c"
	.text
	.globl	cnt
	.bss
	.align 8
	.type	cnt, @object
	.size	cnt, 8
cnt:
	.zero	8
	.section	.rodata
.LC0:
	.string	"usage: %s <niters>\n"
.LC1:
	.string	"BOOM! cnt=%ld\n"
.LC2:
	.string	"OK cnt = %ld \n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB6:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movl	%edi, -36(%rbp)
	movq	%rsi, -48(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	cmpl	$2, -36(%rbp)
	je	.L2
	movq	-48(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, %rsi
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, %edi
	call	exit@PLT
.L2:
	movq	-48(%rbp), %rax
	addq	$8, %rax
	movq	(%rax), %rax
	movq	%rax, %rdi
	call	atoi@PLT
	cltq
	movq	%rax, -32(%rbp)
	leaq	-32(%rbp), %rdx
	leaq	-24(%rbp), %rax
	movq	%rdx, %rcx
	leaq	thread(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	Pthread_create@PLT
	leaq	-32(%rbp), %rdx
	leaq	-16(%rbp), %rax
	movq	%rdx, %rcx
	leaq	thread(%rip), %rdx
	movl	$0, %esi
	movq	%rax, %rdi
	call	Pthread_create@PLT
	movq	-24(%rbp), %rax
	movl	$0, %esi
	movq	%rax, %rdi
	call	Pthread_join@PLT
	movq	-16(%rbp), %rax
	movl	$0, %esi
	movq	%rax, %rdi
	call	Pthread_join@PLT
	movq	-32(%rbp), %rax
	leaq	(%rax,%rax), %rdx
	movq	cnt(%rip), %rax
	cmpq	%rax, %rdx
	je	.L3
	movq	cnt(%rip), %rax
	movq	%rax, %rsi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	jmp	.L4
.L3:
	movq	cnt(%rip), %rax
	movq	%rax, %rsi
	leaq	.LC2(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
.L4:
	movl	$0, %edi
	call	exit@PLT
	.cfi_endproc
.LFE6:
	.size	main, .-main
	.globl	thread
	.type	thread, @function
thread:
.LFB7:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	(%rax), %rax
	movq	%rax, -8(%rbp)
	movq	$0, -16(%rbp)
	jmp	.L7
.L8:
	movq	cnt(%rip), %rax
	addq	$1, %rax
	movq	%rax, cnt(%rip)
	addq	$1, -16(%rbp)
.L7:
	movq	-16(%rbp), %rax
	cmpq	-8(%rbp), %rax
	jl	.L8
	movl	$0, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	thread, .-thread
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
