/** @file wrap_thread_proc.c
 *
 */

#include <thread.h>
#include <thread_table.h>
#include <cond.h>
#include <syscall.h>

void wrap_thread_proc(thr_info *tinfo, int *spinlock, void *(*func)(void *),
                      void *args) {
    int tid = gettid();
    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);

    tinfo->tid = tid;
    thread_table_insert(tid, tinfo);
    *spinlock = 0;

    mutex_unlock(mutex);
    void *status = func(args);
    thr_exit(status);
}

