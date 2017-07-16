/** @file mutex_asm.S
 *  @brief Assembly helpers for mutex locking and releasing.
 *  
 *  The mutex_lock_asm() function takes a mutex address and caller tid
 *  as arguments. It uses the xchg instruction to attempt to acquire
 *  the mutex. On success, it moves the caller tid to the holder_tid
 *  mutex attribute. This is done in assembly to be as quick as possible
 *  and allow subsequent lockers to yield().
 *
 *  The corresponding mutex_unlock_asm() function is a simple exchange.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

.global mutex_lock_asm
.global mutex_unlock_asm

mutex_lock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %ecx */
    movl  8(%esp), %edx     /* store tid in %edx */
    movl  $1, %eax          /* prepare to lock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    testl %eax, %eax        /* test exchange result */
    jne   exit              /* exit if not acquired */
    movl  %edx, 4(%ecx)     /* move tid to mutex->holder_tid */
exit:
    ret

mutex_unlock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %ecx */
    movl  $0, %eax          /* prepare to unlock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    ret

