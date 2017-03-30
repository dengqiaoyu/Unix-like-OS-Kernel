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


.global asm_exec
WRAP_SYSCALL(asm_exec, kern_exec)

.global asm_gettid
WRAP_SYSCALL(asm_gettid, kern_gettid)
