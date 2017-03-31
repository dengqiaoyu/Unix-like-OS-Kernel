/** @file asm_interrupt.S
 *
 */

.globl asm_keyboard_handler
asm_keyboard_handler:
    PUSHA                      /* push all the general register */
    CALL    add_to_kb_buf      /* call the handler */
    POPA                       /* restore all the general register */
    IRET                       /* return */

.globl asm_timer_handler
asm_timer_handler:
    PUSHA            /* push all the general register */
    CALL    sim_breakpoint
    CALL    ticktock /* call the handler */
    POPA             /* restore all the general register */
    IRET             /* return */
