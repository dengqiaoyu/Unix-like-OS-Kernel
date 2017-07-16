/** @file thread_table.h
 *  @brief Contains function prototypes for the thread block table.
 *
 *  This file defines the interface for the thread table to be used in thread.c.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
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

/** @brief Initializes the table for thr_info structs (control blocks).
 *
 *  Allocates memory for the thread table and thread allocator table.
 *  For each entry in the allocator table, initializes an allocator
 *  and points the table entry to the allocator. Creates an array of
 *  mutexes of the same size as the thread table to be fetched by
 *  thread_table_get_mutex(). Initializes a global counter and counter
 *  mutex for allocator distribution.
 *
 *  @return zero on success and negative on failure
 */
int thread_table_init();

/** @brief Allocates a thread block and returns its address.
 *
 *  Reads and increments the global counter to figure out which thread
 *  allocator to use, then fetches memory for a thread block from that
 *  allocator.
 *
 *  @return pointer to thr_info on success and NULL on failure
 */
thr_info *thread_table_alloc();

/** @brief Inserts a thread block into the thread table.
 *
 *  Takes in a thread's tid and a pointer to a thr_info struct with
 *  the thread's control information. Assumes that the thread block
 *  was previously allocated by thread_table_alloc(). Inserts the
 *  thread block at the front of the list at the appropriate table
 *  index. The caller must be holding the corresponding table mutex.
 *
 *  @param tid thread tid to be used for "hashing" into table
 *  @param tinfo pointer to populated thr_info struct (control block)
 *  @return void
 */
void thread_table_insert(int tid, thr_info *tinfo);

/** @brief Finds a thread block from the thread's tid.
 *
 *  Gets a table index from tid and iterates through the table trying
 *  to find the matching thread block. The caller must be holding the
 *  corresponding table mutex.
 *
 *  @param tid thread tid to search for
 *  @return pointer to thr_info on success and NULL on failure
 */
thr_info *thread_table_find(int tid);

/** @brief Deletes a thread block from the thread table.
 *
 *  Removes a thread block from the table and joins the list. Frees the
 *  block memory from the allocator. The caller must be holding the
 *  corresponding table mutex.
 *
 *  @param tinfo pointer to thr_info struct in table
 *  @return void
 */
void thread_table_delete(thr_info *tinfo);

/** @brief Fetches a pointer to the mutex needed to insert, find, or
 *         delete the thread block for the given thread tid.
 *
 *  "Hashes" into the table to get the table index for input thread tid,
 *  then gets the address of the corresponding mutex.
 *
 *  @param tid thread tid to be inserted, found, or deleted
 *  @return pointer to the corresponding table mutex
 */
mutex_t *thread_table_get_mutex(int tid);

#endif
