/**
 * @file cond.c
 * @brief conditianl variable implementation here.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <syscall.h>
#include <malloc.h>
#include "cond_type.h"
#include "cond_type_internal.h"

int dev_cond_init(cond_t *cv) {
    // wait_list_t *wait_list = &(cv->wait_list);
    //@BUG if ini_list cannot malloc more memory.
    cv->wait_list = init_list();
    cv->is_act = 1;
    return SUCCESS;
}

void dev_cond_destory(cond_t *cv) {
    if (cv->is_act == 0 || cv->wait_list->node_cnt != 0)
        return;

    cv->is_act = 0;
}

void dev_cond_wait(cond_t *cv, mutex_t *mp) {
    int tid = gettid();
    mutex_lock(&(cv->mutex));
    wait_list_node_t *node_enqed = cond_enq(cv->wait_list, tid);
    wait_list_item_t *wait_list_item = (wait_list_item_t *)node_enqed->data;
    mutex_unlock(mp);
    mutex_unlock(&(cv->mutex));
    deschedule(&(wait_list_item->is_not_runnable));
    mutex_lock(mp);
    free(node_enqed);
}

void dev_cond_signal(cond_t *cv) {
    mutex_lock(&(cv->mutex));
    wait_list_item_t *wait_list_item = cond_deq(cv->wait_list);
    wait_list_item->is_not_runnable = 1;
    make_runnable(wait_list_item->tid);
    mutex_unlock(&(cv->mutex));
}

void dev_cond_broadcast(cond_t *cv) {
    int i;
    mutex_lock(&(cv->mutex));
    int current_wait_leng = cv->wait_list->node_cnt;
    mutex_unlock(&(cv->mutex));

    for (i = 0; i < current_wait_leng; i++)
        cond_signal(cv);
}

wait_list_node_t *dev_cond_enq(wait_list_t *wait_list, int tid) {
    wait_list_node_t *wait_list_node = calloc(0, sizeof(wait_list_item_t));
    wait_list_item_t *wait_list_item = (wait_list_item_t *)wait_list_node->data;
    wait_list_item->tid = tid;
    wait_list_item->is_not_runnable = 0;
    add_node_to_tail(wait_list, wait_list_node);
    return wait_list_node;
}

wait_list_item_t *dev_cond_deq(wait_list_t *wait_list) {
    return (wait_list_item_t *)(pop_first_node(wait_list)->data);
}

