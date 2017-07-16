/**
 * @file   asm_kern_to_user.S
 * @author Newton Xie (ncw)
 * @bug    No known bugs
 */

#include <x86/seg.h>        /* SEGSEL_USER_DS, SEGSEL_USER_CS */
#include <x86/eflags.h>     /* EFL_IF */

.global kern_to_user
kern_to_user:
    movl    4(%esp), %ecx
    movl    8(%esp), %edx
    movl    $SEGSEL_USER_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    pushl   %eax
    pushl   %ecx
    pushf
    
    popl    %eax
    or      $EFL_IF, %eax
    pushl   %eax

    pushl   $SEGSEL_USER_CS
    pushl   %edx
    iret
