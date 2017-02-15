/** remove_pages.S
 *
 *  Assembly wrapper for remove_pages syscall
 **/

#include <syscall_int.h>

.global remove_pages

remove_pages:
    pushl %ebp                /* store old base pointer */
    movl  %esp, %ebp          /* move new stack base to %ebp */
    push  %esi                /* store %esi (callee-save) */
    movl  8(%ebp), %esi       /* move argument on stack to %esi */
    int   $REMOVE_PAGES_INT   /* trap instruction for remove_pages */
    movl  -4(%ebp), %esi      /* restore %esi */
    movl  %ebp, %esp          /* restore %esp */
    popl  %ebp                /* restore old base pointer */
    ret

