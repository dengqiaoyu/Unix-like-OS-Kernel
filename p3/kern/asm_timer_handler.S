.globl asm_timer_handler

asm_timer_handler:
    PUSHA            /* push all the general register */
    CALL    ticktock /* call the handler */
    POPA             /* restore all the general register */
    IRET             /* return */
