/** asm_exceptions.S
 *
 */

#include <x86/seg.h>
#include <simics.h>

.global asm_pf_handler
asm_pf_handler:
    pushl   %ebp
    movl    %esp, %ebp
    pusha
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs
    movl    $SEGSEL_KERNEL_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    4(%ebp), %eax
    pushl   %eax
    call    pf_handler
    popl    %eax
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popa
    leave
    addl    $4, %esp
    iret
