/** @file thread_table.h
 *
 */

#ifndef _TID_TABLE_H_
#define _TID_TABLE_H_
#include "mutex.h"
#include "cond.h"

#include <mutex.h>
#include <cond.h>

#define RUNNABLE 0
#define SUSPENDED 1
#define EXITED 2

typedef struct thread_info thr_info;
struct thread_info {
    int tid;
    int tid_count;
    void *stack;
    int state;
    int join_tid;
    void *status;
    mutex_t mutex;
    cond_t cond;
};

int thread_table_init(int table_size);
thr_info *thread_table_find(int tid);
int thread_table_delete(thr_info *tinfo);

#endif
