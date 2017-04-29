/**
 * @file   asm_kern_to_user.S
 * @author Newton Xie (ncw)
 * @bug    No known bugs
 */

#include <x86/seg.h>        /* SEGSEL_USER_DS, SEGSEL_USER_CS */
#include <x86/eflags.h>     /* EFL_IF */

#define SEGSEL_GUEST_CS (SEGSEL_SPARE0 | 0x3)
#define SEGSEL_GUEST_DS (SEGSEL_SPARE1 | 0x3)

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

.global kern_to_guest
kern_to_guest:
    movl    4(%esp), %ecx
    movl    $SEGSEL_GUEST_DS, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs

    pushl   $SEGSEL_GUEST_DS
    pushl   $0

    pushf
    popl    %eax
    or      $EFL_IF, %eax
    pushl   %eax

    pushl   $SEGSEL_GUEST_CS
    pushl   %ecx

    pushl   $0
    pushl   $0
    pushl   $0
    pushl   $0
    pushl   $0
    pushl   $0
    pushl   $0
    pushl   $0
    popa

    movl    $0x2badb002, %eax
    movl    $0x15410, %ebx
    iret
