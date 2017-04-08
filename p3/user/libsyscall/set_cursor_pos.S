/** set_cursor_pos.S
 *
 *  Assembly wrapper for set_cursor_pos syscall
 **/

#include <syscall_int.h>

.global set_cursor_pos

set_cursor_pos:
    pushl %ebp                  /* store old base pointer */
    movl  %esp, %ebp            /* move new stack base to %ebp */
    pushl %esi                  /* store %esi (callee-save) */
    lea   8(%ebp), %esi         /* use stack argument build as system call packet */
    int   $SET_CURSOR_POS_INT   /* trap instruction for set_cursor_pos */
    movl  -4(%ebp), %esi        /* restore %esi */
    movl  %ebp, %esp            /* restore %esp */
    popl  %ebp                  /* restore old base pointer */
    ret

