/** set_term_color.S
 *
 *  Assembly wrapper for set_term_color syscall
 **/

#include <syscall_int.h>

.global set_term_color

set_term_color:
    pushl %ebp                /* store old base pointer */
    movl  %esp, %ebp          /* move new stack base to %ebp */
    push  %esi                /* store %esi (callee-save) */
    movl  8(%ebp), %esi       /* move argument on stack to %esi */
    int   $SET_TERM_COLOR_INT /* trap instruction for set_term_color */
    movl  -4(%ebp), %esi      /* restore %esi */
    movl  %ebp, %esp          /* restore %esp */
    popl  %ebp                /* restore old base pointer */
    ret

