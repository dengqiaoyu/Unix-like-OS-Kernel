/** asm_exceptions.S
 *
 */

#include <x86/seg.h>
#include <ureg.h>

#define NO_ERROR_CODE 0
#define ERROR_CODE 1

.global asm_pagefault
asm_pagefault:
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
    call    pagefault_handler
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
    pop     %eax
    add     $4, %esp
    iret

#define WRAP_EXCEPTION(wrapper_name, cause, ec_flag) ;\
wrapper_name: ;\
    push    %eax ;\
    push    %ecx ;\
    push    %edx ;\
    push    %ebx ;\
    push    %esp ;\
    push    %ebp ;\
    push    %esi ;\
    push    %edi ;\
    push    %gs ;\
    push    %fs ;\
    push    %es ;\
    push    %ds ;\
    mov     $SEGSEL_KERNEL_DS, %ax ;\
    mov     %ax, %ds ;\
    mov     %ax, %es ;\
    mov     %ax, %fs ;\
    mov     %ax, %gs ;\
    push    $ec_flag ;\
    push    $cause ;\
    call    exn_handler ;\
    add     $8, %esp ;\
    pop     %ds ;\
    pop     %es ;\
    pop     %fs ;\
    pop     %gs ;\
    pop     %edi ;\
    pop     %esi ;\
    pop     %ebp ;\
    add     $4, %esp ;\
    pop     %ebx ;\
    pop     %edx ;\
    pop     %ecx ;\
    mov     $ec_flag, %eax ;\
    test    %eax, %eax ;\
    je      no_error_code ;\
    pop     %eax ;\
    add     $4, %esp ;\
    iret ;\

no_error_code:
    pop     %eax ;\
    iret

.global asm_divide
WRAP_EXCEPTION(asm_divide, SWEXN_CAUSE_DIVIDE, NO_ERROR_CODE)
