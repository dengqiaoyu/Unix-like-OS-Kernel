/** exec.S
 *
 *  Assembly wrapper for exec syscall
 **/

#include <syscall_int.h>

.global exec

exec:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    pushl %esi            /* store %esi (callee-save) */
    lea   8(%ebp), %esi   /* use stack argument build as system call packet */
    int   $EXEC_INT       /* trap instruction for exec */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

