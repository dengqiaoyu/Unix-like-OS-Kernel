/** asm_page_inval.S
 *
 */

.global asm_page_inval
asm_page_inval:
    movl    4(%esp), %ecx
    invlpg  (%ecx)
    ret
