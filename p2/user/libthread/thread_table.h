/** @file thread_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_
#include "mutex.h"
#include "cond.h"

typedef struct thread_info thr_info;
struct thread_info {
    int counter;
    int kernel_tid;
    void *stack;
    mutex_t mutex;
    cond_t cond;
    int join_tid;
    int state;
    void *status;
};

// status CODE
#define RUNNABLE 0
#define SUSPEND 1
#define EXIT 2

int thread_table_init(int num_thread_lists);
thr_info *thread_table_find(int tid);

#endif
