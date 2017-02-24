/** @file thread_table.c
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <mutex.h>
#include <simics.h>
#include "thr_internals.h"
#include "thread_table.h"
#include "allocator.h"

typedef struct table_node table_node_t;
struct table_node {
    thr_info tinfo;
    table_node_t *prev;
    table_node_t *next;
};

int counter;
mutex_t counter_mutex;
allocator_t **thread_allocators;
table_node_t **thread_table;
mutex_t *table_mutexes;

int thread_table_init() {
    counter = 0;
    if (mutex_init(&counter_mutex) != 0) return -1;

    void *temp = calloc(THREAD_TABLE_SIZE, sizeof(void *));
    thread_table = (table_node_t **)temp;
    if (thread_table == NULL) return -1;

    temp = calloc(NUM_THREAD_ALLOCATORS, sizeof(void *));
    thread_allocators = (allocator_t **)temp;
    if (thread_allocators == NULL) return -1;
    int i;
    for (i = 0; i < NUM_THREAD_ALLOCATORS; i++) {
        allocator_t **allocatorp = &(thread_allocators[i]);
        if (allocator_init(allocatorp, sizeof(table_node_t), THREAD_BLOCK_SIZE))
            return -1;
    }

    table_mutexes = (mutex_t *)calloc(THREAD_TABLE_SIZE, sizeof(mutex_t));
    if (table_mutexes == NULL) return -1;
    for (i = 0; i < THREAD_TABLE_SIZE; i++) {
        if (mutex_init(&(table_mutexes[i])) != 0) return -1;
    }
    return 0;
}

thr_info *thread_table_alloc() {
    mutex_lock(&counter_mutex);
    int count = counter++;
    mutex_unlock(&counter_mutex);

    assert(thread_allocators != NULL);
    allocator_t *allocator = thread_allocators[THREAD_ALLOCATOR_INDEX(count)];
    assert(allocator != NULL);

    thr_info *new_node = allocator_alloc(allocator);
    if (new_node == NULL) return NULL;
    return new_node;
}

void thread_table_insert(int tid, thr_info *tinfo) {
    int tid_list = THREAD_TABLE_INDEX(tid);
    table_node_t *new_node = (table_node_t *)tinfo;

    new_node->prev = NULL;
    if (thread_table[tid_list] == NULL) {
        thread_table[tid_list] = new_node;
        new_node->next = NULL;
    } else {
        thread_table[tid_list]->prev = new_node;
        new_node->next = thread_table[tid_list];
        thread_table[tid_list] = new_node;
    }
}

thr_info *thread_table_find(int tid) {
    int tid_list = THREAD_TABLE_INDEX(tid);
    table_node_t *temp;

    temp = thread_table[tid_list];
    while (temp != NULL) {
        if (temp->tinfo.tid == tid) {
            return &(temp->tinfo);
        } else temp = temp->next;
    }
    return (thr_info *)0;
}

void thread_table_delete(thr_info *tinfo) {
    assert(tinfo != NULL);
    int tid_list = THREAD_TABLE_INDEX(tinfo->tid);
    table_node_t *temp = (table_node_t *)tinfo;

    if (temp->prev != NULL) temp->prev->next = temp->next;
    else thread_table[tid_list] = temp->next;
    if (temp->next != NULL) temp->next->prev = temp->prev;
    allocator_free(temp);
}

mutex_t *thread_table_get_mutex(int tid) {
    assert(table_mutexes != NULL);
    int tid_list = THREAD_TABLE_INDEX(tid);
    return &(table_mutexes[tid_list]);
}

