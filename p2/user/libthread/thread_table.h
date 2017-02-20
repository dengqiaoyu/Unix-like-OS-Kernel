/** @file thread_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_

typedef struct thread_info thr_info;
struct thread_info {
  int tid;
  int count;
  void *stack;
  int state;
  void *status;
  mutex_t mutex;
  cond_t cond;
  thr_info *next;
};

int thread_table_init(int num_thread_lists);
thr_info *thread_table_find(int tid);

#endif
