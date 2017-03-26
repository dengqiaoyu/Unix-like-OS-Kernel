.global asm_get_esp_eip
asm_get_esp_eip:
    push %ecx
    push %edx
    movl 4(%esp), %ecx
    movl 8(%esp), %edx
    movl %esp, (%ecx)
    pusha
    # call cp_kernel_sp
    call get_eip
    movl %eax (%edx)
    popa
    # leal 4(%eax) (%edx) ?
    pop %edx
    pop %ecx
    ret
get_eip:
    mov (%esp), %eax
    ret
