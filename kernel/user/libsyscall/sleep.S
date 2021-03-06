/** sleep.S
 *
 *  Assembly wrapper for sleep syscall
 **/

#include <syscall_int.h>

.global sleep

sleep:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    pushl %esi            /* store %esi (callee-save) */
    movl  8(%ebp), %esi   /* move argument on stack to %esi */
    int   $SLEEP_INT      /* trap instruction for sleep */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

