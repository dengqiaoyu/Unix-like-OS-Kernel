/** asm_syscalls.S
 *
 */

#define WRAP_SYSCALL(wrapper_name, syscall) ;\
wrapper_name: ;\
    push    %ebx ;\
    push    %ecx ;\
    push    %edx ;\
    push    %esi ;\
    push    %edi ;\
    push    %ebp ;\
    call    syscall ;\
    pop     %ebp ;\
    pop     %edi ;\
    pop     %esi ;\
    pop     %edx ;\
    pop     %ecx ;\
    pop     %ebx ;\
    iret ;\

.global asm_gettid
WRAP_SYSCALL(asm_gettid, kern_gettid)

.global asm_exec
WRAP_SYSCALL(asm_exec, kern_exec)

.global asm_fork
WRAP_SYSCALL(asm_fork, kern_fork)

.global asm_new_pages
WRAP_SYSCALL(asm_new_pages, kern_new_pages)

.global asm_deschedule
WRAP_SYSCALL(asm_deschedule, kern_deschedule)
