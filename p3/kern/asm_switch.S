/** asm_switch.S
 *
 */

#include <x86/seg.h>
#include <x86/eflags.h>

.global kern_to_user
kern_to_user:
    movl    $SEGSEL_KERNEL_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    4(%esp), %ecx
    movl    8(%esp), %edx
    pushl   $SEGSEL_KERNEL_DS
    pushl   %ecx
    pushf
    popl    %eax
    or      $EFL_IF, %eax
    pushl   %eax
    pushl   $SEGSEL_KERNEL_CS
    pushl   %edx
    iret
