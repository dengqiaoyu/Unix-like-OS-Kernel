/**
 * @file asm_context_switch.S
 * @brief This file contains three functions that are used to switch to
 *        different threads with different status.
 * @author Newton Xie (ncx) Qiaoyu Deng (qdeng)
 * @bug No known bugs
 */

.global asm_switch_to_runnable
asm_switch_to_runnable:
    pushl   %ebp                 /* save old %ebp */
    movl    %esp, %ebp           /* save esp to %ebp */
    movl    8(%ebp), %eax        /* %eax = &cur_tcb_ptr->cur_sp */
    movl    12(%ebp), %ecx       /* %ecx = new_tcb_ptr->cur_sp */
    pusha                        /* save general registers */
    movl    %esp, (%eax)         /* cur_tcb_ptr->cur_sp = %esp */
    movl    %ecx, %esp           /* %esp = new_tcb_ptr->cur_sp */
    popa                         /* restore all the registers */
    leave                        /* rstore old $ebp and restore %esp */
    ret

.global asm_switch_to_initialized
asm_switch_to_initialized:
    pushl   %ebp                 /* save old %ebp */
    movl    %esp, %ebp           /* save esp to %ebp */
    movl    8(%ebp), %eax        /* %eax = &cur_tcb_ptr->cur_sp */
    movl    12(%ebp), %ecx       /* %ecx = new_tcb_ptr->cur_sp */
    movl    16(%ebp), %edx       /* %edx = new_tcb_ptr->ip */
    pusha                        /* save general registers */
    movl    %esp, (%eax)         /* cur_tcb_ptr->cur_sp = %esp */
    pushl   %edx                 /* second argument for kern_to_user, ip */
    pushl   %ecx                 /* first argument for kern_to_user, sp */
    call    kern_to_user         /* kern_to_user(uint32_t sp, uint32_t ip) */

.global asm_switch_to_forked
asm_switch_to_forked:
    pushl   %ebp                 /* save old %ebp */
    movl    %esp, %ebp           /* save esp to %ebp */
    movl    8(%ebp), %eax        /* %eax = &cur_tcb_ptr->cur_sp */
    movl    12(%ebp), %ecx       /* %ecx = new_tcb_ptr->cur_sp */
    movl    16(%ebp), %edx       /* %edx = new_tcb_ptr->ip */
    pusha                        /* save general registers */
    movl    %esp, (%eax)         /* cur_tcb_ptr->cur_sp = %esp */
    movl    %ecx, %esp           /* %esp = new_tcb_ptr->cur_sp */
    jmp     %edx                 /* %eip = new_tcb_ptr->ip */
