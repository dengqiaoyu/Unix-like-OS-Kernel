/**
 * @file cond_type_internal.h
 * @brief This file contains internal data type define and function declaration
 *        used by condtionnal varibale library.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#ifndef __COND_TYPE_INTERNAL_H__
#define __COND_TYPE_INTERNAL_H__

#include "list.h" /* list_t */

typedef list_t wait_list_t;
typedef node_t wait_list_node_t;

typedef struct wait_list_item_t {
    int tid;
    /* used by deschdule to determine whether get suspended */
    int is_not_runnable;
} wait_list_item_t;

/**
 * Add new thread node into condtionnal varibale list.
 * @param  wait_list list that neeeds to add
 * @param  tid       thread that need to be added into queue
 * @return           the pointer to the new added thread node inside the queue
 */
wait_list_node_t *cond_enq(wait_list_t *wait_list, int tid);

/**
 * Remove the first node in the queue.
 * @param  wait_list list that neeeds to remove
 * @return           the pointer to removed thread node inside the queue
 */
wait_list_item_t *cond_deq(wait_list_t *wait_list);

#endif /* __COND_TYPE_INTERNAL_H__ */