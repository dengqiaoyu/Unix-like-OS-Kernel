#include <stdlib.h>
#include <simics.h>
#include "scheduler.h"   /* tcb_tb_node_t */
#include "task.h"
#include "tcb_hashtab.h"

tcb_hashtab_t tcb_hashtab;

int tcb_hashtab_init() {
    int i;
    for (i = 0; i < HASH_LEN; i++) {
        list_init(&(tcb_hashtab.tcb_list[i]));
        mutex_init(&(tcb_hashtab.mutex[i]));
    }
    return SUCCESS;
}

void tcb_hashtab_put(thread_t *node) {
    int tid = node->tid;
    int idx = tid % HASH_LEN;
    list_t *obj_list = &(tcb_hashtab.tcb_list[idx]);
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);
    tcb_tb_node_t *tcb_tb_node = GET_TAB_NODE_FROM_TCB(node);
    mutex_lock(obj_mutex);
    add_node_to_tail(obj_list, tcb_tb_node);
    mutex_unlock(obj_mutex);
}

thread_t *tcb_hashtab_get(int tid) {
    int idx = tid % HASH_LEN;
    list_t *obj_list = &(tcb_hashtab.tcb_list[idx]);
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);
    mutex_lock(obj_mutex);
    tcb_tb_node_t *node_rover = get_first_node(obj_list);
    if (node_rover == NULL) {
        mutex_unlock(obj_mutex);
        return NULL;
    }
    thread_t *thr_ptr = GET_TCB_FROM_TAB(node_rover);
    if (thr_ptr->tid == tid) {
        mutex_unlock(obj_mutex);
        return thr_ptr;
    }
    while (has_next(obj_list, node_rover) == 1) {
        node_rover = get_next_node(node_rover);
        thread_t *thr_rover = GET_TCB_FROM_TAB(node_rover);
        if (thr_rover->tid == tid) {
            mutex_unlock(obj_mutex);
            return thr_rover;
        }
    }
    mutex_unlock(obj_mutex);
    return NULL;
}

int tcb_hashtab_rmv(int tid) {
    int idx = tid % HASH_LEN;
    list_t *obj_list = &(tcb_hashtab.tcb_list[idx]);
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);
    mutex_lock(obj_mutex);
    tcb_tb_node_t *node_rover = get_first_node(obj_list);
    if (node_rover == NULL) {
        mutex_unlock(obj_mutex);
        return -1;
    }
    thread_t *thr_ptr = GET_TCB_FROM_TAB(node_rover);
    if (thr_ptr->tid == tid) {
        delete_node(obj_list, node_rover);
        mutex_unlock(obj_mutex);
        return 0;
    }
    while (has_next(obj_list, node_rover) == 1) {
        node_rover = get_next_node(node_rover);
        thread_t *thr_rover = GET_TCB_FROM_TAB(node_rover);
        if (thr_rover->tid == tid) {
            delete_node(obj_list, node_rover);
            mutex_unlock(obj_mutex);
            return 0;
        }
    }
    mutex_unlock(obj_mutex);
    return -1;
}