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

extern sche_node_t *cur_sche_node;

int mutex_init(mutex_t *mp) {
    mp->lock = 0;
    // list_init(&mp->list);
    list_init(&mp->list);
    return 0;
}

void mutex_destroy(mutex_t *mp) {
    return;
}

void mutex_lock(mutex_t *mp) {
    // uint32_t eflags = get_eflags();

    disable_interrupts();
    if (mp->lock == 1) {
        MAGIC_BREAK;
        GET_TCB(cur_sche_node)->status = BLOCKED_MUTEX;
        add_node_to_tail(&mp->list, cur_sche_node);

        // if (eflags & EFL_IF)
        enable_interrupts();

        sche_yield();
    } else {
        mp->lock = 1;
        enable_interrupts();
    }
}

void mutex_unlock(mutex_t *mp) {
    // uint32_t eflags = get_eflags();

    disable_interrupts();
    sche_node_t *new_sche_node = pop_first_node(&mp->list);
    if (new_sche_node == NULL) {
        mp->lock = 0;
    } else {
        GET_TCB(new_sche_node)->status = RUNNABLE;
        append_to_scheduler(new_sche_node);
    }

    // if (eflags & EFL_IF)
    enable_interrupts();
}
