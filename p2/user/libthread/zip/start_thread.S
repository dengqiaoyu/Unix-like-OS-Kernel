/** start_thread.S
 *
 *  This is an assembly wrapper which sets up and begins execution of a new
 *  thread. It expects as arguments the highest address of an allocated stack,
 *  the thread procedure, and a pointer to the procedure's argument build.
 *  
 *  The thread_fork syscall is used to create the new thread, and the child
 *  thread's argument is placed on the new stack. The child thread then calls
 *  its procedure and should never return. The parent thread returns the new
 *  child's thread id.
 */

#include <syscall_int.h>

.global start_thread
.global wrap_thread_proc

start_thread:
    pushl %ebp              /* store old base pointer */
    movl  %esp, %ebp        /* move new stack base to %ebp */
    pushl %esi              /* store %esi (callee-save) */
    pushl %edi              /* store %edi (callee-save) */
    movl  8(%ebp), %esi     /* store child stack base in %esi */
    movl  12(%ebp), %edi    /* store thread_info pointer in %edi */
    movl  16(%ebp), %ebx    /* store spinlock pointer in %ebx */
    movl  20(%ebp), %ecx    /* store child thread procedure in %ecx */
    movl  24(%ebp), %edx    /* store child thread parameter in %edx */
    int   $THREAD_FORK_INT  /* trap instruction for thread_fork */
    testl %eax, %eax        /* test thread_fork return value */
    je    child             /* jump if thread_fork returned 0 */
parent:
    movl  -4(%ebp), %esi    /* restore %esi */
    movl  -8(%ebp), %edi    /* restore %edi */
    movl  %ebp, %esp        /* restore %esp */
    popl  %ebp              /* restore old base pointer */
    ret
child:
    movl  %esi, %esp        /* set child stack pointer */
    pushl %edx              /* load child thread parameter argument */
    pushl %ecx              /* load child thread procedure argument */
    pushl %ebx              /* load spinlock pointer argument */
    pushl %edi              /* load thread_info pointer argument */
    call  wrap_thread_proc  /* begin executing child thread procedure */
                            /* wrap_thread_proc should never return */

