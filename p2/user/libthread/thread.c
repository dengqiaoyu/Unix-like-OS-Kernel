/** @file thread.c
 *
 */

#include <stdlib.h>
#include <types.h>
#include "mutex.h"
#include "cond.h"
#include "thr_internals.h"
#include "thread_table.h"
#include "allocator.h"

#define THREAD_TABLE_SIZE 16
#define GET_THREAD_TABLE_INDEX(tid) (tid % THREAD_TABLE_SIZE)
#define NUM_THREAD_ALLOCATORS 16
#define GET_THREAD_ALLOCATOR(tid) (tid % NUM_THREAD_ALLOCATORS)
#define NUM_LIST_CHUNKS 16

// thread_table_lock(tid);
// thread_table_unlock(tid);

int stack_size;
int thread_counter;
mutex_t counter_mutex;
allocator_t **stack_allocators;

int thr_init(int size)
{
    stack_size = size;
    tid_counter = 0;
    if (mutex_init(&counter_mutex) != 0) return -1;

    stack_allocators = (allocator_t **)calloc(NUM_STACK_LISTS, sizeof(void *));
    if (stack_allocators == NULL) return -1;
    int i;
    for (i = 0; i < NUM_THREAD_LISTS; i++) {
        if (allocator_init(&(stack_allocators[i]), stack_size, LIST_CHUNK_NUM))
            return -1;
    }

    if (thread_table_init() < 0) return -1;
    return 0;
}

int thr_create(void *(*func)(void *), void *args)
{
    mutex_lock(&counter_mutex);
    int count = thread_counter++;
    mutex_unlock(&counter_mutex);
    int stack_group = GET_STACK_GROUP(count);

    if (stack_allocators == NULL) return -1; // impossible
    if (stack_allocators[stack_group] == NULL) return -1; // impossible

    void *new_stack = allocator_alloc(stack_allocators[stack_group]);
    if (new_stack == NULL) return -1;
    void *stack_bottom = (void *)((char *)new_stack + stack_size);
    int tid = start_thread(stack_bottom, func, args);

    thr_info *tinfo = thread_table_insert(kernel_tid);
    if (tinfo == NULL) return -1;
    tinfo->tid = tid;
    tinfo->count = count;
    tinfo->stack = new_stack;
    tinfo->status = 0;
    return tid;
}

int thr_join(int tid, void **statusp) {
}

void thr_exit(void *status) {
    int tid = thr_getid();
    thread_t *thr_to_exit = find_thr(tid);
    // Is it impossible to get thread_info?
    thr_to_exit->status = THREADS_EXIT;
    thr_to_exit->return_val = status;

    if (thr_to_exit->join_thr_id > 0) {
        mutex_unlock(thr_to_exit->mutex);
        cond_signal(thr_to_exit->cond);
    }
    mutex_unlock(thr_to_exit->mutex);
}

int thr_getid(void) {
    int kernel_tid = gettid();
    return get_user_tid(kernel_tid);
}

int thr_yield(int user_tid) {
    int kernel_tid = get_kernel_tid(user_tid);
    return yield(kernel_tid);
}
