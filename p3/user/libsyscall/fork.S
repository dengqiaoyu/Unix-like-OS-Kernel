/** fork.S
 *
 *  Assembly wrapper for fork syscall
 **/

#include <syscall_int.h>

.global fork

fork:
    int  $FORK_INT       /* trap instruction for fork */
    ret

