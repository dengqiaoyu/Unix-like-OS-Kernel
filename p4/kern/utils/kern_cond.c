/**
 * @file  kern_cond.c
 * @brief This file contains the basic function implementation for
 *        conditional variable including initialize, destroy, wait and signal.
 * The implementation for conditional variable is just use a FIFO queue, when
 * there is no condition that has been meet, thread get suspended, which means
 * the thread will not be scheduled until condition is meet again, the first
 * node in the list will be waked to continue.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug    No known bugs
 */
#include <stdlib.h>
#include <assert.h>            /* assert */
#include <asm.h>               /* disable_interrupts, enable_interrupts */

#include "utils/kern_cond.h"
#include "utils/list.h"         /* list_t */
#include "scheduler.h"          /* sche_yield */
#include "utils/tcb_hashtab.h"  /* tcb_hashtab_get */

/* internal function */
/**
 * @brief      This is is a helper function used by kern_cond to perform
 *             make_runnable operation, since we cannot call syscall inside
 *             kernel directly.
 * @param  tid the tid of thread that need to waked up
 * @return     0 as success , -1 when thread does not exist or thread is
 *             currently non-runnable due to a call to deschedule()
 */
static int kcond_make_runnable(int tid);

/**
 * Initialize conditionnal varibale, any use of conditionnal varibale should
 * first call cond_init.
 * @param  cv the pointer to the conditional variable
 * @return    0 as succsss, -1 as failure
 */
int kern_cond_init(kern_cond_t *cv) {
    assert(cv != NULL);
    cv->wait_list = list_init();
    if (cv->wait_list == NULL) return -1;
    int ret = kern_mutex_init(&cv->mutex);
    if (ret != 0) {
        list_destroy(cv->wait_list);
        return -1;
    }
    cv->is_active = 1;
    return 0;
}

/**
 * @brief    Destroy the conditional variable.
 * @param cv the pointer to the conditional variable
 */
void kern_cond_destroy(kern_cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_active == 1);
    kern_mutex_lock(&cv->mutex);
    assert(get_list_size(cv->wait_list) == 0);
    list_destroy(cv->wait_list);
    kern_mutex_destroy(&cv->mutex);
    cv->is_active = 0;
}

/**
 * @brief    Wait on a conditional variable, and get deschedule until the
 *           condition is fulfilled.
 * @param cv the pointer to the conditional variable
 * @param mp the mutex that needs to be unlocked before descheduling, and
 *           reacquired after awake.
 */
void kern_cond_wait(kern_cond_t *cv, kern_mutex_t *mp) {
    assert(cv != NULL);
    assert(mp != NULL);
    assert(cv->is_active == 1);
    kern_mutex_lock(&cv->mutex);
    kcond_wlist_node_t wlist_node = {{0}};
    wlist_node.tid = get_cur_tcb()->tid;
    node_t *node = (node_t *)&wlist_node;
    add_node_to_tail(cv->wait_list, node);
    kern_mutex_unlock(mp);
    /**
     * we need disable here because we need to ensure no one can make us
     * before we get suspended
     */
    disable_interrupts();
    kern_mutex_unlock(&cv->mutex);
    sche_yield(SUSPENDED);
    enable_interrupts();
    kern_mutex_lock(mp);
}

/**
 * @brief    Signal a conditional variable, once the condition is meet by some events,
 * @param cv the pointer to the conditional variable
 */
void kern_cond_signal(kern_cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_active == 1);
    kern_mutex_lock(&cv->mutex);
    kcond_wlist_node_t *wlist_node =
        (kcond_wlist_node_t *)pop_first_node(cv->wait_list);
    if (wlist_node == NULL) {
        kern_mutex_unlock(&cv->mutex);
        return;
    }
    kcond_make_runnable(wlist_node->tid);
    kern_mutex_unlock(&cv->mutex);
}

/**
 * @brief      This is is a helper function used by kern_cond to perform
 *             make_runnable operation, since we cannot call syscall inside
 *             kernel directly.
 * @param  tid the tid of thread that need to waked up
 * @return     0 as success , -1 when thread does not exist or thread is
 *             currently non-runnable due to a call to deschedule()
 */
int kcond_make_runnable(int tid) {
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    if (thr->status != SUSPENDED) return -1;
    disable_interrupts();
    thr->status = RUNNABLE;
    sche_push_back(thr);
    enable_interrupts();
    return 0;
}
