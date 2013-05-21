	.file	1 "CharTest.c"
	.rdata
	.align	2
$LC0:
	.ascii	"1234\000"
	.text
	.align	2
	.globl	main
	.ent	main
main:
	.frame	$fp,16,$31		# vars= 8, regs= 1/0, args= 0, gp= 0
	.mask	0x40000000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-16
	sw	$fp,8($sp)
	move	$fp,$sp
	lw	$2,$LC0
	nop
	sw	$2,0($fp)
	lb	$2,1($fp)
	move	$sp,$fp
	lw	$fp,8($sp)
	addiu	$sp,$sp,16
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	main
