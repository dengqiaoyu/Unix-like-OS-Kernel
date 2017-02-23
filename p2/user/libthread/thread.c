/** @file thread.c
 *
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

int stack_size;
int thread_counter;
mutex_t counter_mutex;
allocator_t **stack_allocators;

void set_canaries(void *stack_chunk);
void check_canaries(thr_info *tinfo);

int thr_init(unsigned int size) {
    stack_size = size;
    thread_counter = 0;
    if (mutex_init(&counter_mutex) != 0) return -1;

    void *temp = calloc(NUM_STACK_ALLOCATORS, sizeof(void *));
    stack_allocators = (allocator_t **)temp;
    if (stack_allocators == NULL) return -1;

    int stack_chunk_size = stack_size + 2 * sizeof(int);
    int i;
    for (i = 0; i < NUM_STACK_ALLOCATORS; i++) {
        allocator_t **allocatorp = &(stack_allocators[i]);
        if (allocator_init(allocatorp, stack_chunk_size, STACK_BLOCK_SIZE)) {
            lprintf("line 41 in thr_init\n");
            return -1;
        }
    }

    if (thread_table_init(stack_size) < 0) return -1;

    // anything else we could do here?
    int root_tid = gettid();
    thr_info *tinfo = thread_table_insert(root_tid);
    if (tinfo == NULL) return -1;
    tinfo->tid = root_tid;
    tinfo->counter_value = thread_counter++;
    tinfo->stack = NULL;
    tinfo->state = RUNNABLE;
    tinfo->join_tid = 0;
    if (cond_init(&(tinfo->cond)) != SUCCESS) {
        destroy_allocator(temp);
        thread_table_delete(tinfo);
        lprintf("line 60 in thr_init\n");
        return -1;
    }
    // sim_breakpoint();
    lprintf("line 64 in thr_init\n");
    return 0;
}

int thr_create(void *(*func)(void *), void *args) {
    int mtid = gettid();
    lprintf("thread %d ready to create new thread\n", mtid);
    mutex_lock(&counter_mutex);
    int count = thread_counter++;
    mutex_unlock(&counter_mutex);

    assert(stack_allocators != NULL);
    allocator_t *allocator = stack_allocators[STACK_ALLOCATOR_INDEX(count)];
    assert(allocator != NULL);

    void *stack_chunk = allocator_alloc(allocator);
    if (stack_chunk == NULL) {
        lprintf("line 79 in thr_create\n");
        return -1;
    }
    set_canaries(stack_chunk);

    lprintf("creating thread in line 86 in thr_create as thread %d\n", mtid);

    void *stack_bottom = (void *)((char *)stack_chunk + stack_size + sizeof(int));
    int tid = start_thread(stack_bottom, func, args);

    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);
    thr_info *tinfo = thread_table_insert(tid);
    lprintf("creating thread in line 94 in thr_create as thread %d\n", mtid);
    if (tinfo == NULL) {
        mutex_unlock(mutex);
        return -1;
    }
    tinfo->tid = tid;
    tinfo->counter_value = count;
    tinfo->stack = stack_chunk;
    tinfo->state = RUNNABLE;
    tinfo->join_tid = 0;
    cond_init(&(tinfo->cond));
    if (cond_init(&(tinfo->cond)) != SUCCESS) {
        lprintf("thread %d should not be here\n", mtid);
        mutex_unlock(mutex);
        thread_table_delete(tinfo);
        return -1;
    }
    mutex_unlock(mutex);

    lprintf("thread %d created by %d\n", tid, mtid);
    return tid;
}

int thr_join(int tid, void **statusp) {
    lprintf("thread %d is ready to join %d\n", gettid(), tid);
    mutex_t *mutex = thread_table_get_mutex(tid);

    mutex_lock(mutex);
    lprintf("get mutex\n");

    thr_info *thr_to_join = thread_table_find(tid);
    if (thr_to_join == NULL) {
        lprintf("cannot find thr_info\n");
        mutex_unlock(mutex);
        mutex_lock(&counter_mutex);
        int thread_counter_local = thread_counter;
        mutex_lock(&counter_mutex);
        if (tid > thread_counter_local) return ERROR_THREAD_NOT_CREATED;
        else return ERROR_THREAD_ALREADY_JOINED;
    }
    check_canaries(thr_to_join);
    lprintf("thread %d after get tidinfo for %d\n", gettid(), tid);
    if (thr_to_join->join_tid > 0) {
        mutex_unlock(mutex);
        return ERROR_THREAD_ALREADY_JOINED;
    }
    thr_to_join->join_tid = gettid();
    if (thr_to_join->state != EXITED) {
        cond_wait(&(thr_to_join->cond), mutex);
    }
    if (statusp != NULL) *statusp = thr_to_join->status;

    // *statusp = thr_to_join->status;
    mutex_unlock(mutex);
    lprintf("thread %d after cond_wait for %d\n", gettid(), tid);
    while (1) {
        int ret = make_runnable(tid);
        if (ret < 0) {
            ret = yield(tid);
            if (ret < 0) {
                mutex_lock(mutex);
                break;
            }
        }
    }

    lprintf("thread %d after ensure vanish for %d\n", gettid(), tid);

    // while (yield(tid) == 0) {}
    if (thr_to_join->stack != NULL) allocator_free(thr_to_join->stack);
    thread_table_delete(thr_to_join);
    mutex_unlock(mutex);
    lprintf("thread %d got cleared\n", tid);
    return SUCCESS;
}

void thr_exit(void *status) {
    int tid = gettid();
    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);

    thr_info *thr_to_exit = thread_table_find(tid);

    while (thr_to_exit == NULL) {
        mutex_unlock(mutex);
        yield(-1);
        mutex_lock(mutex);
        thr_to_exit = thread_table_find(tid);
    }
    assert(thr_to_exit != NULL);
    check_canaries(thr_to_exit);
    thr_to_exit->state = EXITED;
    thr_to_exit->status = status;

    if (thr_to_exit->join_tid > 0) {
        mutex_unlock(mutex);
        cond_signal(&(thr_to_exit->cond));
    } else mutex_unlock(mutex);
    lprintf("thread %d exit\n", tid);
    vanish();
}

int thr_getid(void) {
    return gettid();
}

int thr_yield(int tid) {
    return yield(tid);
}

void set_canaries(void *stack_chunk) {
    int *top_canary = (int *)stack_chunk;
    *top_canary = CANARY_VALUE;
    char *temp = (char *)stack_chunk + stack_size + sizeof(int);
    int *bottom_canary = (int *)temp;
    *bottom_canary = CANARY_VALUE;
}

void check_canaries(thr_info *tinfo) {
    assert(tinfo != NULL);
    if (tinfo->stack == NULL) return;

    int *top_canary = (int *)(tinfo->stack);
    if (*top_canary != CANARY_VALUE) {
        lprintf("thread %d: stack may have overflowed\n", tinfo->tid);
    }

    char *temp = (char *)(tinfo->stack) + stack_size + sizeof(int);
    int *bottom_canary = (int *)temp;
    if (*bottom_canary != CANARY_VALUE) {
        lprintf("thread %d: stack may be corrupted\n", tinfo->tid);
    }
}

