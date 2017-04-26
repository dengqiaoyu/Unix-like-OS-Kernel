/** @file kern_mutex.c
 *  @brief Implements mutexes.
 *
 *  @author Newton Xie (ncx)
 *  @author Qiaoyu Deng (qdeng)
 *  @bug none known
 */

#include <stdlib.h>
#include <simics.h>
#include <x86/asm.h>
#include <x86/eflags.h>

#include "scheduler.h"
#include "task.h"
#include "utils/kern_mutex.h"
#include "utils/list.h"

int kern_mutex_init(kern_mutex_t *mp) {
    mp->is_locked = 0;
    mp->blocked_list = list_init();
    if (mp->blocked_list == NULL) return -1;
    return 0;
}

void kern_mutex_destroy(kern_mutex_t *mp) {
    list_destroy(mp->blocked_list);
}

void kern_mutex_lock(kern_mutex_t *mp) {
    thread_t *cur_thread = get_cur_tcb();

    disable_interrupts();
    if (mp->is_locked == 1) {
        thread_t *kmutex_holder = (thread_t *)mp->mutex_holder;
        if (kmutex_holder->status == RUNNABLE)
            sche_move_front(kmutex_holder);
        add_node_to_tail(mp->blocked_list, TCB_TO_SCHE_NODE(cur_thread));
        sche_yield(BLOCKED_MUTEX);
    } else {
        mp->is_locked = 1;
        mp->mutex_holder = (void *)get_cur_tcb();
        enable_interrupts();
    }
}

void kern_mutex_unlock(kern_mutex_t *mp) {
    disable_interrupts();

    sche_node_t *new_sche_node = pop_first_node(mp->blocked_list);
    if (new_sche_node == NULL) {
        mp->is_locked = 0;
        mp->mutex_holder = NULL;
    } else {
        thread_t *new_thread = SCHE_NODE_TO_TCB(new_sche_node);
        new_thread->status = RUNNABLE;
        mp->mutex_holder = (void *)new_thread;
        sche_push_back(new_thread);
    }

    enable_interrupts();
}

void cli_kern_mutex_unlock(kern_mutex_t *mp) {
    sche_node_t *new_sche_node = pop_first_node(mp->blocked_list);
    if (new_sche_node == NULL) {
        mp->is_locked = 0;
        mp->mutex_holder = NULL;
    } else {
        thread_t *new_thread = SCHE_NODE_TO_TCB(new_sche_node);
        new_thread->status = RUNNABLE;
        mp->mutex_holder = (void *)new_thread;
        sche_push_back(new_thread);
    }
}
