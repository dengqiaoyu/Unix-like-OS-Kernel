.globl keyboard_handler

keyboard_handler:
    PUSHA                      /* push all the general register */
    CALL    add_to_kb_buf      /* call the handler */
    POPA                       /* restore all the general register */
    IRET                       /* return */
