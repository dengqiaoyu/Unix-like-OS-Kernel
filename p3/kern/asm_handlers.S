/** asm_handlers.S
 *
 */

#include <x86/seg.h>

.global asm_pf_handler
asm_pf_handler:
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
    call    pf_handler
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popa    
    iret
