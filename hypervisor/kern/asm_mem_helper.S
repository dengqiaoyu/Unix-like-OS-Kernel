.global asm_get_gs
asm_get_gs:
    mov %gs, %ax  /* put register gs to ax as a return value */
    ret

.global asm_set_gs
asm_set_gs:
    mov 4(%esp), %ax
    mov %ax, %gs
    ret

.global asm_copy_dw_from_gs
asm_copy_dw_from_gs:
    movl 4(%esp), %ecx          /* host_dest */
    movl 8(%esp), %edx          /* guest_src */
    movl %gs:(%edx), %eax         /* %eax = (guest_src in %gs0 */
    movl %eax, (%ecx)           /* *host_dest = (guest_src in %gs) */
    ret

.global asm_copy_b_from_gs
asm_copy_b_from_gs:
    movl 4(%esp), %ecx          /* host_dest */
    movl 8(%esp), %edx          /* guest_src */
    movb %gs:(%edx), %al          /* %al = (guest_src in %gs0 */
    movb %al, (%ecx)            /* *host_dest = (guest_src in %gs) */
    ret

.global asm_copy_dw_to_gs
asm_copy_dw_to_gs:
    movl 4(%esp), %ecx          /* guest_dest */
    movl 8(%esp), %edx          /* host_src */
    movl (%edx), %eax           /* %eax = (host_src) */
    movl %eax, %gs:(%ecx)         /* (%ecx in %gs) = (host_src) */
    ret

.global asm_copy_b_to_gs
asm_copy_b_to_gs:
    movl 4(%esp), %ecx          /* guest_dest */
    movl 8(%esp), %edx          /* host_src */
    movl $0, %eax
    movb (%edx), %al           /* %eax = (host_src) */
    movb %al, %gs:(%ecx)         /* (%ecx in %gs) = (host_src) */
    ret
