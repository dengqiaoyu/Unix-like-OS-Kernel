/** halt.S
 *
 *  Assembly wrapper for halt syscall
 **/

#include <syscall_int.h>

.global halt

halt:
    int  $HALT_INT       /* trap instruction for halt */
    ret

