/** @file asm_interrupt.S
 *
 */
#include <x86/seg.h>

.globl asm_keyboard_handler
asm_keyboard_handler:
    pushl   %ebp
    movl    %esp, %ebp
    pusha                      /* push all the general register */
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs
    movl    $SEGSEL_KERNEL_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    call    add_to_kb_buf      /* call the handler */
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popa                       /* restore all the general register */
    leave
    iret                       /* return */

.globl asm_timer_handler
asm_timer_handler:
    pushl   %ebp
    movl    %esp, %ebp
    pusha                      /* push all the general register */
    pushl   %ds
    pushl   %es
    pushl   %fs
    pushl   %gs
    movl    $SEGSEL_KERNEL_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    call    timer_handler      /* call the handler */
    popl    %gs
    popl    %fs
    popl    %es
    popl    %ds
    popa                       /* restore all the general register */
    leave
    iret                       /* return */