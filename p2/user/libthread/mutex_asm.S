/** mutex_asm.S
 *
 */

.global mutex_lock_asm
.global mutex_unlock_asm

mutex_lock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %esi */
    movl  8(%esp), %edx     /* store mutex address in %esi */
    movl  $1, %eax          /* prepare to lock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    testl %eax, %eax
    jne   exit
    movl  %edx, 4(%ecx)
exit:
    ret

mutex_unlock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %esi */
    movl  $0, %eax          /* prepare to unlock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    ret

