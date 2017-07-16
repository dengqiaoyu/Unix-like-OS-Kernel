/** swexn.S
 *
 *  Assembly wrapper for swexn syscall
 **/

#include <syscall_int.h>

.global swexn

swexn:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    pushl %esi            /* store %esi (callee-save) */
    lea   8(%ebp), %esi   /* use stack argument build as system call packet */
    int   $SWEXN_INT      /* trap instruction for swexn */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

