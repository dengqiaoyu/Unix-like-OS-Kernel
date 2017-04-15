/* fork */
.global asm_set_exec_context
asm_set_exec_context:
    pushl %ebx
    movl %esp, %ebx         /* %ebx = old %esp */
    addl $4, %ebx
    pusha
    movl 16(%ebx), %eax
    pushl %eax
    movl %esp, %eax
    movl 4(%ebx), %ecx      /* %ecx = old_kern_sp */
    movl 8(%ebx), %edx      /* %edx = new_kern_sp */
    subl %esp, %ecx         /* len = old_kern_sp - current %esp */
    subl %ecx, %edx         /* new thread %esp = new_kern_sp - len */
    pushl %ecx              /* parameter 3 len */
    pushl %eax              /* parameter 2 old thread esp */
    pushl %edx              /* parameter 1 new thread esp */
    movl 12(%ebx), %eax     /* %eax = &(new_thread->cur_sp) */
    movl %edx, (%eax)       /* *new_thread->cur_sp = new thread %esp */
    call memcpy             /* memcpy(new_thread_sp, old_thread_sp, len) */
    addl $12, %esp          /* restore %esp */
    call asm_get_eip
    popl %edx               /* %edx = &(new_thread->ip) */
    movl %eax, (%edx)       /* new_thread->ip = %eip */
    popa
    popl %ebx
    ret

asm_get_eip:
    movl (%esp), %eax
    ret

.global asm_hlt
asm_hlt:
    HLT

