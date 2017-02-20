/** @file thread_table.c
 *
 */

#include <stdlib.h>
#include <mutex.h>
#include "thread_table.h"

allocator_t **allocators;
thr_info **thread_lists;

int thread_table_init()
{
  allocators = (allocator_t **)calloc(NUM_THREAD_LISTS, sizeof(void *));
  if (allocators == NULL) return -1;
  int i;
  for (i = 0; i < NUM_THREAD_LISTS; i++) {
    if ((allocators[i] = malloc(sizeof(allocator_t))) == NULL) return -1;
    if (allocator_init(allocators[i], sizeof(thr_info), LIST_CHUNK_NUM))
      return -1;
  }
  return 0;
}

thr_info *thread_table_insert(int tid);
{
  int tid_group = GET_THREAD_GROUP(tid);
  if (allocators == NULL) return NULL; // impossible
  if (allocators[tid_group] == NULL) return NULL; // impossible

  thr_info *tinfo = (thr_info *)allocator_alloc(allocators[tid_group]);
  if (tinfo == NULL) return NULL;

  if (thread_lists[tid_group] == NULL) {
    thread_lists[tid_group] = tinfo;
    tinfo->next = NULL;
  }
  else {
    tinfo->next = thread_lists[tid_group]->head;
    thread_lists[tid_group]->head = tinfo;
  }
  return tinfo;
}

thr_info *thread_table_find(int tid)
{
  int tid_group = GET_THREAD_GROUP(tid);
  thr_info *temp;

  temp = thread_lists[tid_group];
  while (temp != NULL) {
    if (temp->tid == tid) return temp;
    else temp = temp->next;
  }
  return (thr_info *)0;
}

