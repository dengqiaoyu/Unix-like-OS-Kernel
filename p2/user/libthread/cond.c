/**
 * @file cond.c
 * @brief This file contains the basic function implementation for
 *        conditionnal varibale including initialize, destroy, wait and signal.
 * The implementation for conditionnal varibale is just use a FIFO queue, when
 * there is no condition that has been meet, thread get deschedule, when the
 * condition is meet again, the first node in the list will be waked to continue
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <syscall.h>            /* gettid, deschedule, make_runnable*/
#include <malloc.h>             /* calloc */
#include <assert.h>             /* assert */
#include "cond_type.h"          /* cond_t, return value */
#include "cond_type_internal.h" /* wait_list_item_t, cond_enq, cond_deq */

/**
 * Initialize conditionnal varibale, any use of conditionnal varibale should
 * first call cond_init.
 * @param  cv the pointer to the conditional variable
 * @return    SUCCESS(0) as succsss, ERROR_CVAR_INIT_FAILED(-1) as fail
 */
int cond_init(cond_t *cv) {
    assert(cv != NULL);
    int ret = init_list(&(cv->wait_list));
    if (ret != SUCCESS) {
        return ERROR_CVAR_INIT_FAILED;
    }
    mutex_init(&(cv->mutex));
    cv->is_act = 1;
    return SUCCESS;
}

/**
 * Destroy the conditional variable.
 * @param cv the pointer to the conditional variable
 */
void cond_destroy(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    /* ensure there is no thread waiting on the conditional variable */
    assert(cv->wait_list.node_cnt == 0);
    cv->is_act = 0;
}

/**
 * Wait on a conditional variable, and get deschedule until the condition is
 * fullfilled.
 * @param cv the pointer to the conditional variable
 * @param mp the mutex that needs to be unlocked before descheduling, and
 *           reacquired after awake.
 */
void cond_wait(cond_t *cv, mutex_t *mp) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    int tid = gettid();
    mutex_lock(&(cv->mutex));
    wait_list_node_t *node_enqed = cond_enq(&(cv->wait_list), tid);
    assert(node_enqed != NULL);
    wait_list_item_t *wait_list_item = (wait_list_item_t *)node_enqed->data;
    /* release the mutex before descheduling */
    mutex_unlock(mp);
    mutex_unlock(&(cv->mutex));
    /*
     * if is_not_runnable is equal to 1, that means some one has cond_signaled
     * this conditional variable and it might deschedule before make_runnable
     * lead to sleep forever, so in this situation deschedule should not let
     * thread get suspended.
     */
    deschedule(&(wait_list_item->is_not_runnable));
    mutex_lock(&(cv->mutex));
    mutex_unlock(&(cv->mutex));
    /* reacquire mutex*/
    mutex_lock(mp);
    free(node_enqed);
}

/**
 * Signal a conditional variable, once the condition is meet by some events,
 * @param cv the pointer to the conditional variable
 */
void cond_signal(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    mutex_lock(&(cv->mutex));
    wait_list_item_t *wait_list_item = cond_deq(&(cv->wait_list));

    /* no thread is found */
    if (wait_list_item == NULL) {
        mutex_unlock(&(cv->mutex));
        return;
    }
    /* used by deschedule in cond_wait() to deternmine whether to sleep */
    wait_list_item->is_not_runnable = 1;
    make_runnable(wait_list_item->tid);
    mutex_unlock(&(cv->mutex));
}

/**
 * Wake up all the threads waiting on the conditional variable at the moment
 * when cond_broadcast() is called.
 * @param cv the pointer to the conditional variable
 */
void cond_broadcast(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    int i;
    mutex_lock(&(cv->mutex));
    int current_wait_leng = cv->wait_list.node_cnt;
    mutex_unlock(&(cv->mutex));

    for (i = 0; i < current_wait_leng; i++)
        cond_signal(cv);
}

/**
 * Add new thread node into condtionnal varibale list.
 * @param  wait_list list that neeeds to add
 * @param  tid       thread that need to be added into queue
 * @return           the pointer to the new added thread node inside the queue
 */
wait_list_node_t *cond_enq(wait_list_t *wait_list, int tid) {
    wait_list_node_t *wait_list_node = calloc(1, sizeof(wait_list_item_t));
    if (wait_list_node == NULL) return NULL;
    wait_list_item_t *wait_list_item = (wait_list_item_t *)wait_list_node->data;
    wait_list_item->tid = tid;
    wait_list_item->is_not_runnable = 0;
    add_node_to_tail(wait_list, wait_list_node);
    return wait_list_node;
}

/**
 * Remove the first node in the queue.
 * @param  wait_list list that neeeds to remove
 * @return           the pointer to removed thread node inside the queue
 */
wait_list_item_t *cond_deq(wait_list_t *wait_list) {
    wait_list_node_t *wait_list_node = pop_first_node(wait_list);
    if (wait_list_node == NULL) return NULL;
    return (wait_list_item_t *)(wait_list_node->data);
}
