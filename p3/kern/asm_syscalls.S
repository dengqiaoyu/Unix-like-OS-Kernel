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

.global asm_remove_pages
WRAP_SYSCALL(asm_remove_pages, kern_remove_pages)

.global asm_wait
WRAP_SYSCALL(asm_wait, kern_wait)

.global asm_vanish
WRAP_SYSCALL(asm_vanish, kern_vanish)

.global asm_set_status
WRAP_SYSCALL(asm_set_status, kern_set_status)

.global asm_get_ticks
WRAP_SYSCALL(asm_get_ticks, kern_get_ticks)

.global asm_sleep
WRAP_SYSCALL(asm_sleep, kern_sleep)

.global asm_print
WRAP_SYSCALL(asm_print, kern_print)

.global asm_set_term_color
WRAP_SYSCALL(asm_set_term_color, kern_set_term_color)

.global asm_set_cursor_pos
WRAP_SYSCALL(asm_set_cursor_pos, kern_set_cursor_pos)

.global asm_get_cursor_pos
WRAP_SYSCALL(asm_get_cursor_pos, kern_get_cursor_pos)
