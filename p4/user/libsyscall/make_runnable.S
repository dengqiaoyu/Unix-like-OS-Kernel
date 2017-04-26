/** make_runnable.S
 *
 *  Assembly wrapper for make_runnable syscall
 **/

#include <syscall_int.h>

.global make_runnable

make_runnable:
    pushl %ebp                /* store old base pointer */
    movl  %esp, %ebp          /* move new stack base to %ebp */
    pushl %esi                /* store %esi (callee-save) */
    movl  8(%ebp), %esi       /* move argument on stack to %esi */
    int   $MAKE_RUNNABLE_INT  /* trap instruction for make_runnable */
    movl  -4(%ebp), %esi      /* restore %esi */
    movl  %ebp, %esp          /* restore %esp */
    popl  %ebp                /* restore old base pointer */
    ret

