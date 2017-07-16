/** @file wrap_thread_proc.c
 *  @brief Housekeeping wrapper for child thread spawn and exit.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <thread.h>
#include <thread_table.h>
#include <mutex.h>
#include <syscall.h>
#include "autostack_internal.h"

/** @brief Wrapper for child thread to call upon creation.
 *  
 *  Called by start_thread() assembly wrapper immediately
 *  after setting up child stack. Responsible for updating
 *  tid in thread control block and inserting into thread
 *  table. Also installs exception handler, begins execution
 *  of func(args), and gives exit status to thr_exit().
 *
 *  @param alt_stack_base exception stack base
 *  @param tinfo thread control block pointer
 *  @param func thread procedure
 *  @param args thread argument pointer
 *  @return void
 */
void wrap_thread_proc(void *alt_stack_base, thr_info *tinfo,
                      void *(*func)(void *), void *args) {
    int tid = gettid();
    mutex_t *mutex = thread_table_get_mutex(tid);
    mutex_lock(mutex);
    tinfo->tid = tid;
    thread_table_insert(tid, tinfo);
    mutex_unlock(mutex);

    install_vanish_gracefully(alt_stack_base);
    void *status = func(args);
    thr_exit(status);
}

