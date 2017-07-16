/**
 * @file   asm_interrupt.S
 * @brief  This file contains driver's entry function for keyboard and timer
 * @author Qiaoyu Deng (qdeng)
 * @bug    No known bugs
 */
/* x86 specific includes */
#include <x86/seg.h>

#define WARP_DRIVER_HANDLER(warpper_name, handler) ;\
warpper_name: ;\
    pusha ;\
    push    %gs ;\
    push    %fs ;\
    push    %es ;\
    push    %ds ;\
    mov     $SEGSEL_KERNEL_DS, %ax ;\
    mov     %ax, %ds ;\
    mov     %ax, %es ;\
    mov     %ax, %fs ;\
    mov     %ax, %gs ;\
    call    handler ;\
    pop     %ds ;\
    pop     %es ;\
    pop     %fs ;\
    pop     %gs ;\
    popa;\
    iret;\

.globl asm_keyboard_handler
WARP_DRIVER_HANDLER(asm_keyboard_handler, add_to_kb_buf)

.globl asm_timer_handler
WARP_DRIVER_HANDLER(asm_timer_handler, timer_handler)
