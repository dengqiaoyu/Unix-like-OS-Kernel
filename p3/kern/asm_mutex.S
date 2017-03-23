/** @file mutex_asm.S
 *  @brief Assembly helpers for mutex locking and releasing.
 *  
 *  Simply uses the xchg instruction.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

.global mutex_lock_asm
.global mutex_unlock_asm

mutex_lock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %ecx */
    movl  $1, %eax          /* prepare to lock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    ret

mutex_unlock_asm:
    movl  4(%esp), %ecx     /* store mutex address in %ecx */
    movl  $0, %eax          /* prepare to unlock */
    xchg  %eax, (%ecx)      /* exchange %eax with mutex */
    ret

