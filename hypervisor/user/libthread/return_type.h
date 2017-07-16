/** @file return_type.h
 *  @brief Defines return types for thread library functions.
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#ifndef _RETURN_TYPE_H_
#define _RETURN_TYPE_H_

#define SUCCESS 0

/* thread.c */
#define ERROR_THREAD_ALREADY_JOINED -1
#define ERROR_THREAD_NOT_FOUND -2

/* allocator.h */
#define ERROR_ALLOCATOR_INIT_FAILED -1
#define ERROR_ALLOCATOR_ADD_BLOCK_FAILED -2
#define ERROR_ALLOCATOR_MALLOC_NEW_BLOCK_FAILED -3

/* list.h */
#define ERROR_INIT_LIST_CALLOC_FAILED -1

/* cond.h */
#define ERROR_COND_INIT_FAILED -1

/* sem.h */
#define ERROR_SEM_INIT_FAILED -1

/* rwlock.h */
#define ERROR_RWLOCK_INIT_FAILED -1
#define ERROR_RWLOCK_DESTROY_FAILED -2

#endif

