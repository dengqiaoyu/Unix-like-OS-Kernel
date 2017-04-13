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

.global asm_fork
WRAP_SYSCALL(asm_fork, kern_fork)

.global asm_exec
WRAP_SYSCALL(asm_exec, kern_exec)

.global asm_wait
WRAP_SYSCALL(asm_wait, kern_wait)

.global asm_yield
WRAP_SYSCALL(asm_yield, kern_yield)

.global asm_deschedule
WRAP_SYSCALL(asm_deschedule, kern_deschedule)

.global asm_make_runnable
WRAP_SYSCALL(asm_make_runnable, kern_make_runnable)

.global asm_gettid
WRAP_SYSCALL(asm_gettid, kern_gettid)

.global asm_new_pages
WRAP_SYSCALL(asm_new_pages, kern_new_pages)

.global asm_readline
WRAP_SYSCALL(asm_readline, kern_readline)

.global asm_set_status
WRAP_SYSCALL(asm_set_status, kern_set_status)

.global asm_vanish
WRAP_SYSCALL(asm_vanish, kern_vanish)
