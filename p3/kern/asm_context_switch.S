.global asm_switch_to_runnable
.global asm_switch_to_initizlized
.global asm_switch_to_forked

asm_switch_to_runnable:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax
    movl    12(%ebp), %ecx
    pusha
    movl    %esp, (%eax)
    movl    %ecx, %esp
    popa
    leave
    ret

asm_switch_to_initizlized:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax
    movl    12(%ebp), %ecx
    movl    16(%ebp), %edx
    pusha
    movl    %esp, (%eax)
    pushl   %edx
    pushl   %ecx
    call    kern_to_user

asm_switch_to_forked:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %eax
    movl    12(%ebp), %ecx
    movl    16(%ebp), %edx
    pusha
    movl    %esp, (%eax)
    movl    %ecx, %esp
    jmp     %edx