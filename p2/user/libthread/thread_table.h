/** @file thread_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_

#include <mutex.h>
#include <cond.h>

#define RUNNABLE 0
#define SUSPENDED 1
#define EXITED 2

typedef struct thread_info thr_info;
struct thread_info {
    int tid;
    void *stack;
    int state;
    int join_tid;
    void *status;
    cond_t cond;
};

int thread_table_init();
thr_info *thread_table_alloc();
void thread_table_insert(int tid, thr_info *tinfo);
thr_info *thread_table_find(int tid);
void thread_table_delete(thr_info *tinfo);
mutex_t *thread_table_get_mutex(int tid);

#endif
