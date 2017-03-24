/** asm_syscalls.S
 *
 */

.global asm_exec
asm_exec:
    pusha
    call    kern_exec
    popa
    movl    $-2, %eax
    iret

.global asm_gettid
asm_gettid:
    push    %ebx
    push    %ecx
    push    %edx
    push    %esi
    push    %edi
    push    %ebp
    call    kern_gettid
    pop     %ebp
    pop     %edi
    pop     %esi
    pop     %edx
    pop     %ecx
    pop     %ebx
    iret
