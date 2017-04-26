/**
 * @file   asm_page_inval.S
 * @author Newton Xie (ncw)
 * @bug    No known bugs
 */

.global asm_page_inval
asm_page_inval:
    movl    4(%esp), %ecx   /* address that needs to be flushed from TLB */
    invlpg  (%ecx)
    ret
