/** exec.S
 *
 *  Assembly wrapper for exec syscall
 **/

#include <syscall_int.h>

.global gettid

gettid:
    int   $GETTID_INT       /* trap instruction for exec */
    ret

