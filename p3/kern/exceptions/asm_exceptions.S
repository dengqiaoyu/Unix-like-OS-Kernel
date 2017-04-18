/** asm_exceptions.S
 *
 */

#include <ureg.h>
#include <x86/seg.h>
#include <x86/idt.h>

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

#define WRAP_EXN(wrapper_name, cause, ec_flag, handler) ;\
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
    call    handler ;\
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
WRAP_EXN(asm_divide, SWEXN_CAUSE_DIVIDE, NO_ERROR_CODE, exn_handler)

.global asm_debug
WRAP_EXN(asm_debug, SWEXN_CAUSE_DEBUG, NO_ERROR_CODE, exn_handler)

.global asm_break_point
WRAP_EXN(asm_break_point, SWEXN_CAUSE_BREAKPOINT, NO_ERROR_CODE, exn_handler)

.global asm_overflow
WRAP_EXN(asm_overflow, SWEXN_CAUSE_OVERFLOW, NO_ERROR_CODE, exn_handler)

.global asm_boundcheck
WRAP_EXN(asm_boundcheck, SWEXN_CAUSE_BOUNDCHECK, NO_ERROR_CODE, exn_handler)

.global asm_opcode
WRAP_EXN(asm_opcode, SWEXN_CAUSE_OPCODE, NO_ERROR_CODE, exn_handler)

.global asm_nofpu
WRAP_EXN(asm_nofpu, SWEXN_CAUSE_NOFPU, NO_ERROR_CODE, exn_handler)

.global asm_segfault
WRAP_EXN(asm_segfault, SWEXN_CAUSE_SEGFAULT, ERROR_CODE, exn_handler)

.global asm_stackfault
WRAP_EXN(asm_stackfault, SWEXN_CAUSE_STACKFAULT, ERROR_CODE, exn_handler)

.global asm_protfault
WRAP_EXN(asm_protfault, SWEXN_CAUSE_PROTFAULT, ERROR_CODE, exn_handler)

.global asm_fpufault
WRAP_EXN(asm_fpufault, SWEXN_CAUSE_FPUFAULT, NO_ERROR_CODE, exn_handler)

.global asm_alignfault
WRAP_EXN(asm_alignfault, SWEXN_CAUSE_ALIGNFAULT, ERROR_CODE, exn_handler)

.global asm_simdfault
WRAP_EXN(asm_simdfault, SWEXN_CAUSE_SIMDFAULT, NO_ERROR_CODE, exn_handler)

.global asm_nonmaskable
WRAP_EXN(asm_nonmaskable, IDT_NMI, NO_ERROR_CODE, hwerror_handler)

.global asm_coproc
WRAP_EXN(asm_coproc, IDT_CSO, NO_ERROR_CODE, hwerror_handler)

.global asm_task_segf
WRAP_EXN(asm_task_segf, IDT_TS, NO_ERROR_CODE, hwerror_handler)

.global asm_machine_check
WRAP_EXN(asm_machine_check, IDT_MC, NO_ERROR_CODE, hwerror_handler)
