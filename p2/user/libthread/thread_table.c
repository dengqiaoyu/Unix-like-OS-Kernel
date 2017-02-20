/** @file thread_table.c
 *
 */

#include <stdlib.h>
#include <mutex.h>
#include "thr_internals.h"
#include "thread_table.h"

allocator_t **thread_allocators;
thr_info **thread_lists;
mutex_t *list_mutexes;

int thread_table_init()
{
    thread_allocators = (allocator_t **)calloc(NUM_THREAD_LISTS, sizeof(void *));
    if (thread_allocators == NULL) return -1;
    int i;
    for (i = 0; i < NUM_THREAD_LISTS; i++) {
        allocator_t **ptr = &(thread_allocators[i]);
        if (allocator_init(ptr, sizeof(thr_info), LIST_CHUNK_NUM))
            return -1;
    }

    list_mutexes = (mutex_t *)calloc(NUM_THREAD_LISTS, sizeof(mutex_t));
    if (list_mutexes == NULL) return -1;
    for (i = 0; i < NUM_THREAD_LISTS; i++) {
        if (mutex_init(&(list_mutexes[i])) != 0) return -1;
    }
    return 0;
}

thr_info *thread_table_insert(int kernel_tid);
{
    int tid_list = GET_THREAD_LIST(kernel_tid);
    if (thread_allocators == NULL) return NULL; // impossible
    if (thread_allocators[tid_list] == NULL) return NULL; // impossible

    mutex_lock(&(list_mutexes[tid_list]));
    thr_info *tinfo = (thr_info *)allocator_alloc(thread_allocators[tid_list]);
    if (tinfo == NULL) return NULL;

    if (thread_lists[tid_list] == NULL) {
        thread_lists[tid_list] = tinfo;
        tinfo->next = NULL;
    }
    else {
        tinfo->next = thread_lists[tid_list]->head;
        thread_lists[tid_list]->head = tinfo;
    }
    mutex_unlock(&(list_mutexes[tid_list]));
    return tinfo;
}

thr_info *thread_table_find(int kernel_tid)
{
    int tid_list = GET_THREAD_LIST(kernel_tid);
    thr_info *temp;

    mutex_lock(&(list_mutexes[tid_list]));
    temp = thread_lists[tid_list];
    while (temp != NULL) {
        if (temp->kernel_tid == kernel_tid) {
            mutex_unlock(&(list_mutexes[tid_list]));
            return temp;
        }
        else temp = temp->next;
    }
    mutex_unlock(&(list_mutexes[tid_list]));
    return (thr_info *)0;
}

