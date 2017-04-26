/** get_ticks.S
 *
 *  Assembly wrapper for get_ticks syscall
 **/

#include <syscall_int.h>

.global get_ticks

get_ticks:
    int   $GET_TICKS_INT   /* trap instruction for get_ticks */
    ret

