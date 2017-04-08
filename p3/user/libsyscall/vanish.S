/** vanish.S
 *
 *  Assembly wrapper for vanish syscall
 **/

#include <syscall_int.h>

.global vanish

vanish:
    int   $VANISH_INT     /* trap instruction for vanish */
    ret

