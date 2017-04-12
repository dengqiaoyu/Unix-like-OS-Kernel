/** @file mutex.c
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
#include "mutex.h"
#include "list.h"

/* assembly helpers
int mutex_lock_asm(mutex_t *mp);
int mutex_unlock_asm(mutex_t *mp);
*/

int mutex_init(mutex_t *mp) {
    mp->lock = 0;
    mp->blocked_list = list_init();
    if (mp->blocked_list == NULL) return -1;
    return 0;
}

void mutex_destroy(mutex_t *mp) {
    list_destroy(mp->blocked_list);
}

void mutex_lock(mutex_t *mp) {
    thread_t *cur_thread = get_cur_tcb();

    disable_interrupts();

    if (mp->lock == 1) {
        add_node_to_tail(mp->blocked_list, TCB_TO_SCHE_NODE(cur_thread));
        sche_yield(BLOCKED_MUTEX);
    } else {
        mp->lock = 1;
        enable_interrupts();
    }
}

void mutex_unlock(mutex_t *mp) {
    disable_interrupts();

    sche_node_t *new_sche_node = pop_first_node(mp->blocked_list);
    if (new_sche_node == NULL) {
        mp->lock = 0;
    } else {
        thread_t *new_thread = SCHE_NODE_TO_TCB(new_sche_node);
        new_thread->status = RUNNABLE;
        sche_push_back(new_thread);
    }

    enable_interrupts();
}
