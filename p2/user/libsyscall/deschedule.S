/** deschedule.S
 *
 *  Assembly wrapper for deschedule syscall
 **/

#include <syscall_int.h>

.global deschedule

deschedule:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    push  %esi            /* store %esi (callee-save) */
    movl  8(%ebp), %esi   /* move argument on stack to %esi */
    int   $DESCHEDULE_INT /* trap instruction for deschedule */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

