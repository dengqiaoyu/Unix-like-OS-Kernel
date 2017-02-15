/** task_vanish.S
 *
 *  Assembly wrapper for task_vanish syscall
 **/

#include <syscall_int.h>

.global task_vanish

task_vanish:
    pushl %ebp                 /* store old base pointer */
    movl  %esp, %ebp           /* move new stack base to %ebp */
    push  %esi                 /* store %esi (callee-save) */
    movl  8(%ebp), %esi        /* move argument on stack to %esi */
    int   $TASK_VANISH_INT     /* trap instruction for task_vanish */
    movl  -4(%ebp), %esi       /* restore %esi */
    movl  %ebp, %esp           /* restore %esp */
    popl  %ebp                 /* restore old base pointer */
    ret

