.global asm_set_exec_context
asm_set_exec_context:
    pusha
    movl 12(%esp), %eax  /* %eax = uint32_t *new_curr_esp_ptr */
    movl %esp, (%eax) /* *new_curr_esp_ptr = %esp */
    movl 4(%esp), %ecx /* uint32_t old_kern_sp */
    movl 8(%esp), %edx /* uint32_t new_kern_sp */
    sub %ecx, %esp /* %ecx(len) = old_kern_sp - %esp */
    sub %edx, %ecx /* %edx = new_kern_sp - %ecx(len) */
    pushl %esp /* char *dest = %esp */
    pushl %edx /* char *src = %edx */
    pushl %ecx /* len */
    call memcpy
    popl %eax
    popl %eax
    popl %eax
    call get_eip
    movl 16(%esp), %ecx /* uint32_t *new_curr_eip_ptr */
    movl %eax, (%ecx) /* *new_curr_eip_ptr = the addr of "movl 16(%esp) %ecx" */
    popa
    ret
get_eip:
    movl (%esp), %eax
    ret

