/** asm_exceptions.S
 *
 */

#include <x86/seg.h>

.global asm_pf_handler
asm_pf_handler:
    pusha
    call    pf_handler
    popa
    iret
