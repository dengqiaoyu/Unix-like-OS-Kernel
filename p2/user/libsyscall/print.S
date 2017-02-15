/** print.S
 *
 *  Assembly wrapper for print syscall
 **/

#include <syscall_int.h>

.global print

print:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    push  %esi            /* store %esi (callee-save) */
    lea   8(%ebp), %esi   /* use stack argument build as system call packet */
    int   $PRINT_INT      /* trap instruction for print */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

