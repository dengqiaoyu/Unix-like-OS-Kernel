/** set_status.S
 *
 *  Assembly wrapper for set_status syscall
 **/

#include <syscall_int.h>

.global set_status

set_status:
    pushl %ebp                /* store old base pointer */
    movl  %esp, %ebp          /* move new stack base to %ebp */
    pushl %esi                /* store %esi (callee-save) */
    movl  8(%ebp), %esi       /* move argument on stack to %esi */
    int   $SET_STATUS_INT     /* trap instruction for set_status */
    movl  -4(%ebp), %esi      /* restore %esi */
    movl  %ebp, %esp          /* restore %esp */
    popl  %ebp                /* restore old base pointer */
    ret

