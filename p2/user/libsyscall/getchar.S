/** getchar.S
 *
 *  Assembly wrapper for getchar syscall
 **/

#include <syscall_int.h>

.global getchar

getchar:
    int  $GETCHAR_INT       /* trap instruction for getchar */
    ret

