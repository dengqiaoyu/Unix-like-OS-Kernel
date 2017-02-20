/** @file thread_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_

typedef struct thread_info thr_info;
struct thread_info {
  int tid;
  int stack_group;
  void *stack;
  cond_t cv;
  int status;
  void *retval;
  struct node *next;
};

int thread_table_init(int num_thread_lists);
thr_info *thread_table_find(int tid);

#endif
