/** asm_registers.S
 *
 */

.global get_esi
get_esi:
    movl    %esi, %eax
    ret
