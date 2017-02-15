/** new_pages.S
 *
 *  Assembly wrapper for new_pages syscall
 **/

#include <syscall_int.h>

.global new_pages

new_pages:
    pushl %ebp            /* store old base pointer */
    movl  %esp, %ebp      /* move new stack base to %ebp */
    push  %esi            /* store %esi (callee-save) */
    lea   8(%ebp), %esi   /* use stack argument build as system call packet */
    int   $NEW_PAGES_INT  /* trap instruction for new_pages */
    movl  -4(%ebp), %esi  /* restore %esi */
    movl  %ebp, %esp      /* restore %esp */
    popl  %ebp            /* restore old base pointer */
    ret

