/** yield.S
 *
 *  Assembly wrapper for yield syscall
 **/

#include <syscall_int.h>

.global yield

yield:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    push  %esi            /* store %esi (callee-save) */
    movl  8(%ebp), %esi   /* move argument on stack to %esi */
    int   $YIELD_INT      /* trap instruction for yield */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

