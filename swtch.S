# Context switch
#
# void swtch(struct context **old, struct context *new);
#
# save current register context in old
# and then load register context from new
.global swtch
swtch:
	movl 4(%esp), %eax			# old context
	movl 8(%esp), %edx			# new context

	# save old callee-save registers
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi

	# switch stacks
	movl %esp, (%eax)
	movl %edx, %esp

	# Load new callee-save registers
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret

