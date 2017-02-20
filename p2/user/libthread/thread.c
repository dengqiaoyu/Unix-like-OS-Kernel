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
#include "error_type.h"

#define NUM_THREAD_LISTS 16
#define GET_THREAD_LIST(tid) (tid % NUM_THREAD_LISTS)
#define NUM_STACK_LISTS 16
#define GET_STACK_GROUP(tid) (tid % NUM_STACK_LISTS)
#define LIST_CHUNK_NUM 16

int stack_size;
int stack_counter;
mutex_t counter_mutex;
mutex_t *group_mutexes;
allocator_t **stack_allocators;

int thr_init(int size)
{
  stack_size = size;
  tid_counter = 0;
  if (mutex_init(&tid_mutex) != 0) return -1;

  stack_allocators = (allocator_t **)calloc(NUM_THREAD_LISTS, sizeof(void *));
  if (stack_allocators == NULL) return -1;
  int i;
  for (i = 0; i < NUM_THREAD_LISTS; i++) {
    if ((stack_allocators[i] = malloc(sizeof(allocator_t))) == NULL) return -1;
    if (allocator_init(stack_allocators[i], stack_size, LIST_CHUNK_NUM))
      return -1;
  }

  group_mutexes = (mutex_t *)calloc(num_lists, sizeof(mutex_t));
  if (group_mutexes == NULL) return -1;
  for (i = 0; i < num_lists; i++) {
    if (mutex_init(&(group_mutexes[i])) != 0) return -1;
  }

  if (thread_table_init() < 0) return -1;
  return 0;
}

int thr_create(void *(*func)(void *), void *args)
{
  mutex_lock(&counter_mutex);
  int stack_count = stack_counter++;
  mutex_unlock(&counter_mutex);

  int stack_group = GET_STACK_GROUP(stack_count);
  if (stack_allocators == NULL) return -1; // impossible
  if (stack_allocators[stack_group] == NULL) return -1; // impossible

  mutex_lock(&(group_mutexes[stack_group]));
  void *new_stack = allocator_alloc(stack_allocators[stack_group]);
  if (new_stack == NULL) return -1;
  void *stack_bottom = (void *)((char *)new_stack + stack_size);
  int tid = start_thread(stack_bottom, func, args);

  thr_info *tinfo = thread_table_insert(tid);
  if (tinfo == NULL) return -1;
  tinfo->tid = tid;
  tinfo->stack_group = stack_group;
  tinfo->stack = new_stack;
  tinfo->exited = 0;
  mutex_unlock(&(group_mutexes[stack_group]));
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
