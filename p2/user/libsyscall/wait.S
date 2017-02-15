/** wait.S
 *
 *  Assembly wrapper for wait syscall
 **/

#include <syscall_int.h>

.global wait

wait:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    push  %esi            /* store %esi (callee-save) */
    movl  8(%ebp), %esi   /* move argument on stack to %esi */
    int   $WAIT_INT       /* trap instruction for wait */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

