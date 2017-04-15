/** asm_switch.S
 *
 */

#include <x86/seg.h>
#include <x86/eflags.h>

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
    or      $EFL_IOPL_RING3, %eax
    pushl   %eax

    pushl   $SEGSEL_USER_CS
    pushl   %edx
    iret
