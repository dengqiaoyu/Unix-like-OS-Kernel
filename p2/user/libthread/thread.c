/** @file thread.c
 *
 */

#include <stdlib.h>
#include <types.h>
#include "mutex.h"
#include "cond.h"
#include "thr_internals.h"
#include "tid_table.h"
#include "allocator.h"
#include "error_type.h"

#define MAX_THREADS 4096

unsigned int stack_size;
struct tid_table *table;

int thr_init( unsigned int size ) {
    stack_size = size;
    table = (struct tid_table *)calloc(1, sizeof(struct tid_table));
    if (table == NULL) return -1;
    return 0;
}

int thr_create( void *(*func)(void *), void *args ) {
    void *new_stack = malloc(stack_size);
    if (new_stack == NULL) return -1;
    new_stack = (void *)((char *)new_stack + stack_size);

    int tid = start_thread(new_stack, func, args);
    return tid;
}

int thr_join(int tid, void **statusp) {
    thread_info *thr_to_join = thread_table_find(tid);
    if (thr_to_join == NULL) {
        return ERROR_THREADS_COULD_NOT_FIND_BY_ID;
    }
    if (thr_to_join->join_thr_id > 0) {
        mutex_unlock(thr_to_join->mutex);
        return ERROR_THREADS_ALREADY_JOINED;
    }

    thr_to_join->join_thr_id == thr_getid();
    // THREAD_EXITED, THREAD_CLEARED
    if (thr_to_join->status != THREAD_EXIT) {
        cond_wait(thr_to_join->cond, thr_to_join->mutex);
    }

    if (thr_to_join->return_val != NULL) {
        *statusp = thr_to_join->return_val;
    }
    mutex_unlock(thr_to_join->mutex);
    allocator_free(thr_to_join->stack_base);
    allocator_free(thr_to_join);
    delete_thr(tid);
    return SUCCESS;
}

void thr_exit( void *status ) {
    int tid = thr_getid();
    thread_info *thr_to_exit = thread_table_find(tid);
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
