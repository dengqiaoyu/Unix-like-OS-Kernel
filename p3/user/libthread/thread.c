/** @file thread.c
 *  @brief Implements the thread management interface.
 *
 *  The thread management functions rely on the thread table
 *  (defined in thread_table.c) and block allocators (defined in
 *  allocator.c).
 *
 *  The thr_init() function creates and initializes an allocator
 *  table, where each entry points to an allocator that will give
 *  chunks of memory large enough for a thread stack, an alternate
 *  exception stack, and two stack canaries.
 *
 *  The thr_create() function uses the stack allocators and the
 *  thread block table allocator interface to prepare memory and
 *  then transfers control to wrap_thread_proc(), which is an
 *  assembly wrapper for setting up the new thread. The child
 *  thread is responsible for adding itself to the thread table,
 *  installing an exception handler, calling its thread procedure,
 *  and invoking thr_exit() before returning.
 *
 *  The thr_join() function makes several checks to determine the
 *  status of its thread parameter. If the thread is joinable, it
 *  reaps the thread and reads its exit status, blocking first if
 *  necessary.
 *
 *  The thr_exit() function simply writes to the control block.
 *  The thr_yield() and thr_getid() functions are simply calls to
 *  kernel defined system calls, since the library uses kernel
 *  defined thread numbers.
 *
 *  Calls to thr_join() and thr_exit() attempt to detect thread
 *  stack overflow by checking stack canaries. If overflow is
 *  detected, an error message is printed.
 *
 *  @author Qiaoyu Deng (qdeng)
 *  @author Newton Xie (ncx)
 *  @bug none known
 */

#include <stdlib.h>
#include <stdio.h>
#include <types.h>
#include <assert.h>
#include <simics.h>
#include <syscall.h>
#include <thread.h>
#include <mutex.h>
#include <cond.h>

#include "thr_internals.h"
#include "thread_table.h"
#include "allocator.h"
#include "return_type.h"

int stack_size;
unsigned int counter;
mutex_t counter_mutex;
allocator_t **stack_allocators;

/** @brief Initializes the thread library.
 *
 *  Allocates memory for the stack allocator table, and points each
 *  table entry to an initialized allocator. Sets a global counter
 *  and counter mutex to distribute stack allocation evenly among the
 *  allocators. Initializes the thread table and inserts a node for
 *  the root thread.
 *
 *  @param size stack size for child threads
 *  @return zero on success and negative on failure
 */
int thr_init(unsigned int size) {
    stack_size = size;
    counter = 0;
    if (mutex_init(&counter_mutex) != 0) return -1;

    stack_allocators = calloc(NUM_STACK_ALLOCATORS, sizeof(void *));
    if (stack_allocators == NULL) return -1;

    int stack_chunk_size = stack_size + 2 * sizeof(int) + ALT_STACK_SIZE;
    int i;
    for (i = 0; i < NUM_STACK_ALLOCATORS; i++) {
        allocator_t **allocatorp = &(stack_allocators[i]);
        if (allocator_init(allocatorp, stack_chunk_size, STACK_ALLOCATOR_SIZE))
            return -1;
    }

    if (thread_table_init() < 0) return -1;

    int root_tid = gettid();
    thr_info *tinfo = thread_table_alloc();
    if (tinfo == NULL) return -1;
    tinfo->tid = root_tid;
    tinfo->stack = NULL;
    tinfo->state = RUNNABLE;
    tinfo->join_tid = 0;
    if (cond_init(&(tinfo->cond)) != SUCCESS) return -1;
    thread_table_insert(root_tid, tinfo);
    return 0;
}

/** @brief Forks a thread and returns its tid to the parent.
 *
 *  Reads and increments the global counter in order to choose a
 *  stack allocator to ask for memory. On success, places canaries
 *  on the stack and calculates the addresses for the "bottoms" of
 *  the child thread stack and the alternate exception stack.
 *
 *  Allocates and initializes a thread control block, then passes
 *  the block address, stack addresses, and procedure arguments to
 *  start_thread, which forks and sets up the child thread.
 *
 *  @param func child thread procedure
 *  @param args child thread argument pointer
 *  @return child thread tid on success and negative on failure
 */
int thr_create(void *(*func)(void *), void *args) {
    mutex_lock(&counter_mutex);
    int count = counter++;
    mutex_unlock(&counter_mutex);

    assert(stack_allocators != NULL);
    allocator_t *allocator = stack_allocators[STACK_ALLOCATOR_INDEX(count)];
    assert(allocator != NULL);

    void *stack_chunk = allocator_alloc(allocator);
    if (stack_chunk == NULL) return -1;
    set_canaries(stack_chunk);
    void *stack_base = (char *)stack_chunk + stack_size + sizeof(int);
    void *alt_stack_base = (char *)stack_base + sizeof(int) + ALT_STACK_SIZE;

    thr_info *tinfo = thread_table_alloc();
    if (tinfo == NULL) return -1;

    tinfo->stack = stack_chunk;
    tinfo->state = RUNNABLE;
    tinfo->join_tid = 0;
    if (cond_init(&(tinfo->cond)) < 0) return -1;

    int tid = start_thread(stack_base, alt_stack_base, tinfo, func, args);
    return tid;
}

/** brief Cleans up after a thread, blocking if necessary.
 *
 *  Locks a thread table mutex and searches for thread tid's control
 *  block. If the thread block is not found, yields to the thread
 *  until the thread block can be found or it is determined that the
 *  thread has already been joined and removed from the table. In the
 *  latter case, an error is returned.
 *
 *  If no other thread has joined on thread tid, the thread can be
 *  "claimed" by setting join_tid in the control block. If the thread
 *  is still running, then progress is blocked until the thread exits.
 *
 *  Once thread tid can be safely joined, the status parameter is
 *  written to (if it is not NULL), and the joined thread control
 *  block is freed along with its stack and removed from the table.
 *
 *  @param tid thread tid to be joined
 *  @param statusp pointer to address which will receive exit status
 *  @return zero on success and negative on failure
 */
int thr_join(int tid, void **statusp) {
    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);

    thr_info *thr_to_join = thread_table_find(tid);
    while (thr_to_join == NULL) {
        // thread block not found
        // already joined? or yet to insert itself?
        mutex_unlock(mutex);
        int ret = yield(tid);
        mutex_lock(mutex);
        if (ret != 0) {
            // could not yield to thread
            // already joined? or thread blocked?
            thr_to_join = thread_table_find(tid);
            if (thr_to_join == NULL) {
                // already joined!
                mutex_unlock(mutex);
                return ERROR_THREAD_NOT_FOUND;
            }
            // blocked!
            break;
        }
        // yet to insert itself!
        thr_to_join = thread_table_find(tid);
    }
    check_canaries(thr_to_join);
    if (thr_to_join->join_tid > 0) {
        // already joined, but not yet exited
        mutex_unlock(mutex);
        return ERROR_THREAD_ALREADY_JOINED;
    }
    thr_to_join->join_tid = gettid();
    if (thr_to_join->state != EXITED) cond_wait(&(thr_to_join->cond), mutex);

    if (statusp != NULL) *statusp = thr_to_join->status;
    if (thr_to_join->stack != NULL) {
        // stack is never NULL unless joining root thread
        allocator_free(thr_to_join->stack);
    }
    thread_table_delete(thr_to_join);
    mutex_unlock(mutex);
    return SUCCESS;
}

/** @brief Exits a thread.
 *
 *  Acquires a table lock and searches for its own control block
 *  (which should always be found). Sets thread status and exit
 *  status, then vanishes.
 *
 *  @param status exit status
 *  @return void
 */
void thr_exit(void *status) {
    int tid = gettid();
    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);

    thr_info *thr_to_exit = thread_table_find(tid);

    assert(thr_to_exit != NULL);
    check_canaries(thr_to_exit);
    thr_to_exit->state = EXITED;
    thr_to_exit->status = status;

    if (thr_to_exit->join_tid > 0) {
        mutex_unlock(mutex);
        cond_signal(&(thr_to_exit->cond));
    } else mutex_unlock(mutex);
    vanish();
}

/** @brief Returns caller thread tid.
 *
 *  @return caller tid
 */
int thr_getid(void) {
    return gettid();
}

/** @brief Yields to thread tid.
 *
 *  Fails if thread does not exist or is not runnable.
 *
 *  @param tid thread tid to yield to
 *  @return zero on success and negative on failure
 */
int thr_yield(int tid) {
    return yield(tid);
}

/** @brief Sets canaries on thread stack top and bottom.
 *
 *  Given a pointer to a stack memory chunk, places one byte canaries
 *  on the top and bottom of the thread stack.
 *
 *  @param stack_chunk pointer to memory return by stack allocator
 *  @return void
 */
void set_canaries(void *stack_chunk) {
    int *top_canary = (int *)stack_chunk;
    *top_canary = CANARY_VALUE;
    char *temp = (char *)stack_chunk + stack_size + sizeof(int);
    int *bottom_canary = (int *)temp;
    *bottom_canary = CANARY_VALUE;
}

/** @brief Prints error messages if stack canaries are corrupted.
 *
 *  @param tinfo pointer to thread control block to check
 *  @return void
 */
void check_canaries(thr_info *tinfo) {
    assert(tinfo != NULL);
    if (tinfo->stack == NULL) return;

    int *top_canary = (int *)(tinfo->stack);
    if (*top_canary != CANARY_VALUE)
        lprintf("thread %d: stack may have overflowed\n", tinfo->tid);

    char *temp = (char *)(tinfo->stack) + stack_size + sizeof(int);
    int *bottom_canary = (int *)temp;
    if (*bottom_canary != CANARY_VALUE)
        lprintf("thread %d: stack may be corrupted\n", tinfo->tid);
}

