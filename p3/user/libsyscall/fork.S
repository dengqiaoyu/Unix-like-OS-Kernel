/** exec.S
 *
 *  Assembly wrapper for exec syscall
 **/

#include <syscall_int.h>

.global fork

fork:
    int   $FORK_INT       /* trap instruction for exec */
    ret

