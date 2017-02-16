/** get_cursor_pos.S
 *
 *  Assembly wrapper for get_cursor_pos syscall
 **/

#include <syscall_int.h>

.global get_cursor_pos

get_cursor_pos:
    pushl %ebp                  /* store old base pointer */
    movl  %esp, %ebp            /* move new stack base to %ebp */
    pushl %esi                  /* store %esi (callee-save) */
    lea   8(%ebp), %esi         /* use stack argument build as system call packet */
    int   $GET_CURSOR_POS_INT   /* trap instruction for get_cursor_pos */
    movl  -4(%ebp), %esi        /* restore %esi */
    movl  %ebp, %esp            /* restore %esp */
    popl  %ebp                  /* restore old base pointer */
    ret

