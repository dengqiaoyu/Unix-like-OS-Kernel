/**
 * @file  tcb_hashtab.c
 * @brief This file contains the helper functions for searching thread control
 *        block quickly by using simple hash function.
 * @author Qiaoyu Deng
 * @bug No known bugs
 */

#include <stdlib.h>
#include <simics.h>

#include "utils/tcb_hashtab.h"
#include "scheduler.h"          /* tcb_tb_node_t */
#include "task.h"               /* thread_t */
#include "utils/list.h"         /* list_t */

/* used to store thread control block */
static tcb_hashtab_t tcb_hashtab;

/**
 * @brief  Initialize the tcb hash table by creating list and mutex.
 * @return 0 as success, -1 as failure
 */
int tcb_hashtab_init() {
    int i;
    for (i = 0; i < HASH_LEN; i++) {
        tcb_hashtab.tcb_list[i] = list_init();
        if (tcb_hashtab.tcb_list[i] == NULL) return -1;
        int ret = kern_mutex_init(&(tcb_hashtab.mutex[i]));
        if (ret != 0) return -1;
    }
    return 0;
}

/**
 * @brief Put the thread control block into tcb hash table
 * @param tcb pointer of thread control block that needs to be put into
 */
void tcb_hashtab_put(thread_t *tcb) {
    int tid = tcb->tid;
    int idx = tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    kern_mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);
    tcb_tb_node_t *tcb_tb_node = TCB_TO_TABLE_NODE(tcb);
    kern_mutex_lock(obj_mutex);
    add_node_to_tail(obj_list, tcb_tb_node);
    kern_mutex_unlock(obj_mutex);
}

/**
 * @brief      Get tcb according to its tid
 * @param  tid thread's tid
 * @return     pointer to tcb when find it in table, NULL when cannot find it
 */
thread_t *tcb_hashtab_get(int tid) {
    int idx = tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    kern_mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);

    kern_mutex_lock(obj_mutex);
    tcb_tb_node_t *node_rover = get_first_node(obj_list);
    while (node_rover != NULL) {
        thread_t *thr_rover = TABLE_NODE_TO_TCB(node_rover);
        if (thr_rover->tid == tid) {
            kern_mutex_unlock(obj_mutex);
            return thr_rover;
        }
        node_rover = get_next_node(obj_list, node_rover);
    }
    kern_mutex_unlock(obj_mutex);
    return NULL;
}

/**
 * @brief     Remove thread control block from hash table.
 * @param tcb the pointer to tcb
 */
void tcb_hashtab_rmv(thread_t *tcb) {
    int idx = tcb->tid % HASH_LEN;
    list_t *obj_list = tcb_hashtab.tcb_list[idx];
    kern_mutex_t *obj_mutex = &(tcb_hashtab.mutex[idx]);

    kern_mutex_lock(obj_mutex);
    remove_node(obj_list, TCB_TO_TABLE_NODE(tcb));
    kern_mutex_unlock(obj_mutex);
}
