/**
 * @file cond.c
 * @brief conditianl variable implementation here.
 * @author Newton Xie(nwx) Qiaoyu Deng(qdeng)
 * @bug No known bugs
 */

#include <syscall.h>
#include <malloc.h>
#include <assert.h>
#include <simics.h>
#include <stdio.h>
#include "cond_type.h"
#include "cond_type_internal.h"

int cond_init(cond_t *cv) {
    cv->wait_list = init_list();
    mutex_init(&(cv->mutex));
    cv->is_act = 1;
    return SUCCESS;
}

void cond_destroy(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    assert(cv->wait_list->node_cnt == 0);
    cv->is_act = 0;
}

void cond_wait(cond_t *cv, mutex_t *mp) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
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

void cond_signal(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    mutex_lock(&(cv->mutex));
    wait_list_item_t *wait_list_item = cond_deq(cv->wait_list);

    if (wait_list_item == NULL) {
        mutex_unlock(&(cv->mutex));
        return;
    }

    wait_list_item->is_not_runnable = 1;
    make_runnable(wait_list_item->tid);
    mutex_unlock(&(cv->mutex));
}

void cond_broadcast(cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_act = 1);
    int i;
    mutex_lock(&(cv->mutex));
    int current_wait_leng = cv->wait_list->node_cnt;
    mutex_unlock(&(cv->mutex));

    for (i = 0; i < current_wait_leng; i++)
        cond_signal(cv);
}

wait_list_node_t *cond_enq(wait_list_t *wait_list, int tid) {
    wait_list_node_t *wait_list_node = calloc(1, sizeof(wait_list_item_t));
    wait_list_item_t *wait_list_item = (wait_list_item_t *)wait_list_node->data;
    wait_list_item->tid = tid;
    wait_list_item->is_not_runnable = 0;
    add_node_to_tail(wait_list, wait_list_node);
    return wait_list_node;
}

wait_list_item_t *cond_deq(wait_list_t *wait_list) {
    wait_list_node_t *wait_list_node = pop_first_node(wait_list);
    if (wait_list_node == NULL) return NULL;
    return (wait_list_item_t *)(wait_list_node->data);
}

