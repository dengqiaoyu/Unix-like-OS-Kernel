/** misbehave.S
 *
 *  Assembly wrapper for misbehave syscall
 **/

#include <syscall_int.h>

.global misbehave

misbehave:
    pushl %ebp                /* store old base pointer */
    movl  %esp, %ebp          /* move new stack base to %ebp */
    pushl %esi                /* store %esi (callee-save) */
    movl  8(%ebp), %esi       /* move argument on stack to %esi */
    int   $MISBEHAVE_INT      /* trap instruction for misbehave */
    movl  -4(%ebp), %esi      /* restore %esi */
    movl  %ebp, %esp          /* restore %esp */
    popl  %ebp                /* restore old base pointer */
    ret

