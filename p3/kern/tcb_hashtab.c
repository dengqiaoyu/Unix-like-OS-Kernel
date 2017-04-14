#include <stdlib.h>
#include <simics.h>
#include "scheduler.h"   /* tcb_tb_node_t */
#include "task.h"
#include "tcb_hashtab.h"
#include "list.h"

tcb_hashtab_t tcb_hashtab;

int tcb_hashtab_init() {
    int i;
    for (i = 0; i < HASH_LEN; i++) {
        tcb_hashtab.tcb_list[i] = list_init();
        mutex_init(&(tcb_hashtab.mutex[i]));
    }
    return SUCCESS;
}

void tcb_hashtab_put(thread_t *tcb) {
    int tid = tcb->tid;
    int idx = tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);
    tcb_tb_node_t *tcb_tb_node = TCB_TO_TABLE_NODE(tcb);
    mutex_lock(obj_mutex);
    add_node_to_tail(obj_list, tcb_tb_node);
    mutex_unlock(obj_mutex);
}

thread_t *tcb_hashtab_get(int tid) {
    int idx = tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);

    mutex_lock(obj_mutex);
    tcb_tb_node_t *node_rover = get_first_node(obj_list);
    while (node_rover != NULL) {
        thread_t *thr_rover = TABLE_NODE_TO_TCB(node_rover);
        if (thr_rover->tid == tid) {
            mutex_unlock(obj_mutex);
            return thr_rover;
        }
        node_rover = get_next_node(obj_list, node_rover);
    }
    mutex_unlock(obj_mutex);
    return NULL;
}

int tcb_hashtab_rmv(int tid) {
    int idx = tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);

    mutex_lock(obj_mutex);
    tcb_tb_node_t *node_rover = get_first_node(obj_list);
    while (node_rover != NULL) {
        thread_t *thr_rover = TABLE_NODE_TO_TCB(node_rover);
        if (thr_rover->tid == tid) {
            delete_node(obj_list, node_rover);
            mutex_unlock(obj_mutex);
            return 0;
        }
        node_rover = get_next_node(obj_list, node_rover);
    }
    mutex_unlock(obj_mutex);
    return -1;
}
