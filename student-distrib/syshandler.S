#define ASM 1

#include "syshandler.h"
#include "x86_desc.h"

# In particular, the call number is placed in EAX, the rest argument in EBX, then ECX, and finally EDX. N

.global syscallhandle

syscallhandle:
	cmpl	$10, %eax	# check if system call is valid
	ja		invalid_call
	cmpl	$1, %eax
	jb		invalid_call

	pushal
	pushl	%edx	# push arguments in correct order to c-style call
	pushl	%ecx
	pushl	%ebx

	call 	*sys_call_table(,%eax,4)

	popl	%ebx
	popl	%ecx
	popl	%edx
	movl	%eax, 28(%esp)
	popal

	iret
invalid_call:
	movl	$-1, %eax	# return value for error
	iret 

sys_call_table : .long sys_zero, sys_halt, sys_execute ,sys_read ,sys_write ,sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
