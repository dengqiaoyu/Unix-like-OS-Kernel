/** asm_page_inval.S
 *
 */

.global page_inval
page_inval:
    movl    4(%esp), %ecx
    invlpg  (%ecx)
    ret
