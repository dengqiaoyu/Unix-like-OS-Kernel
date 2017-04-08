/** gettid.S
 *
 *  Assembly wrapper for gettid syscall
 **/

#include <syscall_int.h>

.global gettid

gettid:
    int  $GETTID_INT       /* trap instruction for gettid */
    ret

