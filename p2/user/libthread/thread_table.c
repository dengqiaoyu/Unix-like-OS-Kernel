/** @file thread_table.c
 *
 */

#include <stdlib.h>
#include <mutex.h>
#include "thr_internals.h"
#include "thread_table.h"

allocator_t **thread_allocators;
thr_info **thread_table;
mutex_t *table_mutexes;

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

    table_mutexes = (mutex_t *)calloc(THREAD_TABLE_SIZE, sizeof(mutex_t));
    if (table_mutexes == NULL) return -1;
    for (i = 0; i < THREAD_TABLE_SIZE; i++) {
        if (mutex_init(&(table_mutexes[i])) != 0) return -1;
    }

    thread_table = (thr_info **)calloc(THREAD_TABLE_SIZE, sizeof(mutex_t));
    if (table_mutexes == NULL) return -1;
    return 0;
}

thr_info *thread_table_insert(int kernel_tid);
{
    int tid_list = GET_THREAD_LIST(kernel_tid);
    if (thread_allocators == NULL) return NULL; // impossible
    if (thread_allocators[tid_list] == NULL) return NULL; // impossible

    thr_info *tinfo = (thr_info *)allocator_alloc(thread_allocators[tid_list]);
    if (tinfo == NULL) return NULL;

    mutex_lock(&(table_mutexes[tid_list]));
    if (thread_table[tid_list] == NULL) {
        thread_table[tid_list] = tinfo;
        tinfo->next = NULL;
    }
    else {
        tinfo->next = thread_table[tid_list]->head;
        thread_table[tid_list]->head = tinfo;
    }
    mutex_unlock(&(table_mutexes[tid_list]));
    return tinfo;
}

thr_info *thread_table_find(int kernel_tid)
{
    int tid_list = GET_THREAD_LIST(kernel_tid);
    thr_info *temp;

    mutex_lock(&(table_mutexes[tid_list]));
    temp = thread_table[tid_list];
    while (temp != NULL) {
        if (temp->kernel_tid == kernel_tid) {
            mutex_unlock(&(table_mutexes[tid_list]));
            return temp;
        }
        else temp = temp->next;
    }
    mutex_unlock(&(table_mutexes[tid_list]));
    return (thr_info *)0;
}

int thread_table_delete(thr_info *tinfo)
{
    int tid_list = GET_THREAD_LIST(kernel_tid);
    thr_info *temp;

    mutex_lock(&(table_mutexes[tid_list]));
    temp = thread_table[tid_list];
    while (temp != NULL) {
        if (temp->kernel_tid == kernel_tid) {
            mutex_unlock(&(table_mutexes[tid_list]));
            return temp;
        }
        else temp = temp->next;
    }
    mutex_unlock(&(table_mutexes[tid_list]));
    return (thr_info *)0;
}

