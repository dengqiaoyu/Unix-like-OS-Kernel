/** @file thr_internals.h
 *  @brief Defines tuning parameters and internal functions for thread.c.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <thread_table.h>

#define THREAD_TABLE_SIZE 256
#define THREAD_TABLE_INDEX(tid) ((tid) % THREAD_TABLE_SIZE)

#define NUM_THREAD_ALLOCATORS 64
#define THREAD_ALLOCATOR_INDEX(tid) ((tid) % NUM_THREAD_ALLOCATORS)
#define THREAD_ALLOCATOR_SIZE 8

#define NUM_STACK_ALLOCATORS 64
#define STACK_ALLOCATOR_INDEX(tid) ((tid) % NUM_STACK_ALLOCATORS)
#define STACK_ALLOCATOR_SIZE 8

#define ALT_STACK_SIZE 256
#define CANARY_VALUE 0xfeedbeef

/** @brief Forks a child thread and sets up the child stack.
 *
 *  Wraps thread creation process in assembly. Calls thread_fork
 *  and sets child thread %esp to child stack base. Child thread
 *  then calls wrap_thread_proc with appropriate parameters and
 *  should never return. Returns child thread tid to parent.
 *
 *  @param stack_base child thread stack base
 *  @param alt_stack_base child thread exception stack base
 *  @param tinfo child thread control block pointer
 *  @param func child thread procedure
 *  @param args child thread argument pointer
 *  @return child thread tid to parent
 */
int start_thread(void *stack_base, void *alt_stack_base, thr_info *tinfo,
                 void *(*func)(void *), void *args);

/* spec in wrap_thread_proc.c */
void wrap_thread_proc(void *alt_stack_base, thr_info *tinfo,
                      void *(*func)(void *), void *args);

/* spec in thread.c */
void set_canaries(void *stack_chunk);

/* spec in thread.c */
void check_canaries(thr_info *tinfo);

#endif /* THR_INTERNALS_H */

