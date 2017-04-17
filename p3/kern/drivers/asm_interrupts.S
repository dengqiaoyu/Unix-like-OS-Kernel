/** @file asm_interrupt.S
 *
 */
#include <x86/seg.h>

.globl asm_keyboard_handler
asm_keyboard_handler:
    pusha                      /* push all the general register */
    push    %gs
    push    %fs
    push    %es
    push    %ds
    mov     $SEGSEL_KERNEL_DS, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    call    add_to_kb_buf      /* call the handler */
    pop     %ds
    pop     %es
    pop     %fs
    pop     %gs
    popa                       /* restore all the general register */
    iret                       /* return */

.globl asm_timer_handler
asm_timer_handler:
pusha                      /* push all the general register */
    push    %gs
    push    %fs
    push    %es
    push    %ds
    mov     $SEGSEL_KERNEL_DS, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    call    timer_handler      /* call the handler */
    pop     %ds
    pop     %es
    pop     %fs
    pop     %gs
    popa                       /* restore all the general register */
    iret                       /* return */
