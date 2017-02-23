/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#define THREAD_TABLE_SIZE 256
#define THREAD_TABLE_INDEX(tid) (tid % THREAD_TABLE_SIZE)

#define NUM_THREAD_ALLOCATORS 16
#define THREAD_ALLOCATOR_INDEX(tid) (tid % NUM_THREAD_ALLOCATORS)
#define THREAD_BLOCK_SIZE 32

#define NUM_STACK_ALLOCATORS 16
#define STACK_ALLOCATOR_INDEX(tid) (tid % NUM_STACK_ALLOCATORS)
#define STACK_BLOCK_SIZE 32

#define CANARY_VALUE 0xfeedbeef

#define ERROR_THREAD_ALREADY_JOINED -1
#define ERROR_THREAD_NOT_CREATED -2

#define SUCCESS 0

int start_thread( void *stackaddr, void *(*func)(void *), void *args );
void wrap_thread_proc(void *(*func)(void *), void *args);

#endif /* THR_INTERNALS_H */

