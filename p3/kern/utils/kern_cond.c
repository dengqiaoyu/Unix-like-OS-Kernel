#include <stdlib.h>
#include <assert.h>
#include <asm.h>        /* disable_interrupts, enable_interrupts */

#include "utils/kern_cond.h"
#include "utils/list.h"
#include "scheduler.h"
#include "utils/tcb_hashtab.h"

int kcond_make_runnable(int tid);

int kern_cond_init(kern_cond_t *cv) {
    assert(cv != NULL);
    cv->wait_list = list_init();
    assert(cv->wait_list != NULL);
    int ret = mutex_init(&cv->mutex);
    assert(ret == 0);
    cv->is_active = 1;
    return 0;
}

void kern_cond_wait(kern_cond_t *cv, mutex_t *mp) {
    assert(cv != NULL);
    assert(mp != NULL);
    assert(cv->is_active == 1);
    mutex_lock(&cv->mutex);
    kcond_wlist_node_t wlist_node = {{0}};
    wlist_node.tid = get_cur_tcb()->tid;
    node_t *node = (node_t *)&wlist_node;
    add_node_to_tail(cv->wait_list, node);
    mutex_unlock(mp);
    disable_interrupts();
    mutex_unlock(&cv->mutex);
    sche_yield(SUSPENDED);
    enable_interrupts();
    mutex_lock(mp);
}

void kern_cond_signal(kern_cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_active == 1);
    mutex_lock(&cv->mutex);
    kcond_wlist_node_t *wlist_node =
        (kcond_wlist_node_t *)pop_first_node(cv->wait_list);
    if (wlist_node == NULL) {
        mutex_unlock(&cv->mutex);
        return;
    }
    kcond_make_runnable(wlist_node->tid);
    mutex_unlock(&cv->mutex);
}

void kern_cond_destroy(kern_cond_t *cv) {
    assert(cv != NULL);
    assert(cv->is_active == 1);
    mutex_lock(&cv->mutex);
    assert(get_list_size(cv->wait_list) == 0);
    list_destroy(cv->wait_list);
    mutex_destroy(&cv->mutex);
    cv->is_active = 0;
}

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
