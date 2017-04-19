/** asm_syscalls.S
 *
 */
#include <x86/seg.h>

#include <x86/seg.h>

#define WRAP_SYSCALL(wrapper_name, syscall) ;\
wrapper_name: ;\
    push    %ebx ;\
    push    %ecx ;\
    push    %edx ;\
    push    %esi ;\
    push    %edi ;\
    push    %ebp ;\
    push    %gs ;\
    push    %fs ;\
    push    %es ;\
    push    %ds ;\
    mov     $SEGSEL_KERNEL_DS, %ax ;\
    mov     %ax, %ds ;\
    mov     %ax, %es ;\
    mov     %ax, %fs ;\
    mov     %ax, %gs ;\
    call    syscall ;\
    pop     %ds ;\
    pop     %es ;\
    pop     %fs ;\
    pop     %gs ;\
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

.global asm_remove_pages
WRAP_SYSCALL(asm_remove_pages, kern_remove_pages)

.global asm_sleep
WRAP_SYSCALL(asm_sleep, kern_sleep)

.global asm_readline
WRAP_SYSCALL(asm_readline, kern_readline)

.global asm_print
WRAP_SYSCALL(asm_print, kern_print)

.global asm_set_term_color
WRAP_SYSCALL(asm_set_term_color, kern_set_term_color)

.global asm_set_cursor_pos
WRAP_SYSCALL(asm_set_cursor_pos, kern_set_cursor_pos)

.global asm_get_cursor_pos
WRAP_SYSCALL(asm_get_cursor_pos, kern_get_cursor_pos)

.global asm_thread_fork
WRAP_SYSCALL(asm_thread_fork, kern_thread_fork)

.global asm_get_ticks
WRAP_SYSCALL(asm_get_ticks, kern_get_ticks)

.global asm_halt
WRAP_SYSCALL(asm_halt, kern_halt)

.global asm_set_status
WRAP_SYSCALL(asm_set_status, kern_set_status)

.global asm_vanish
WRAP_SYSCALL(asm_vanish, kern_vanish)

.global asm_readfile
WRAP_SYSCALL(asm_readfile, kern_readfile)

.global asm_swexn
asm_swexn:
    push    %eax
    push    %ecx
    push    %edx
    push    %ebx
    push    %esp
    push    %ebp
    push    %esi
    push    %edi
    push    %gs
    push    %fs
    push    %es
    push    %ds
    mov     $SEGSEL_KERNEL_DS, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    call    kern_swexn
    pop     %ds
    pop     %es
    pop     %fs
    pop     %gs
    pop     %edi
    pop     %esi
    pop     %ebp
    add     $4, %esp
    pop     %ebx
    pop     %edx
    pop     %ecx
    cmp     $1, %eax    // if kern_swexn returns 1, we need to pop %eax
    jne     keep_eax
    pop     %eax
    iret
keep_eax:
    add     $4, %esp
    iret

/* syscall helper function for getting arguments */
.global asm_get_esi
asm_get_esi:
    movl    %esi, %eax
    ret
