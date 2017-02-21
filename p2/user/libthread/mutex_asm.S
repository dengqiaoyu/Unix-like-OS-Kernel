/** mutex_asm.S
 *
 */

.global mutex_lock
.global mutex_unlock

mutex_lock:
    pushl %ebp              /* store old base pointer */
    movl  %esp, %ebp        /* move new stack base to %ebp */
    pushl %esi              /* store %esi (callee-save) */
    movl  8(%ebp), %esi     /* store mutex address in %esi */
    movl  $1, %eax          /* prepare to lock */
loop:
    xchg  %eax, (%esi)      /* exchange %eax with mutex */
    testl %eax, %eax        /* test exchanged value */
    jne   loop              /* loop if %eax > 0 (lock not acquired) */
    movl  -4(%ebp), %esi    /* restore %esi */
    movl  %ebp, %esp        /* restore %esp */
    popl  %ebp              /* restore old base pointer */
    ret

mutex_unlock:
    pushl %ebp              /* store old base pointer */
    movl  %esp, %ebp        /* move new stack base to %ebp */
    pushl %esi              /* store %esi (callee-save) */
    movl  8(%ebp), %esi     /* store mutex address in %esi */
    movl  $0, %eax          /* prepare to unlock */
    xchg  %eax, (%esi)      /* exchange %eax with mutex */
    movl  -4(%ebp), %esi    /* restore %esi */
    movl  %ebp, %esp        /* restore %esp */
    popl  %ebp              /* restore old base pointer */
    ret

