#include <stdlib.h>
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */

#include "syscalls/syscalls.h"
#include "task.h"                /* task thread declaration and interface */
#include "scheduler.h"           /* scheduler declaration and interface */
#include "utils/tcb_hashtab.h"         /* insert and find tcb by tid */
#include "drivers/timer_driver.h"        /* get_timer_ticks */


int kern_gettid(void) {
    thread_t *thread = get_cur_tcb();
    return thread->tid;
}

int kern_yield(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int tid = (int)esi;
    if (tid < -1) return -1;
    if (tid == -1) {
        sche_yield(RUNNABLE);
        return 0;
    }
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    disable_interrupts();
    if (thr->status != RUNNABLE) {
        enable_interrupts();
        return -1;
    }
    sche_push_front(thr);
    sche_yield(RUNNABLE);
    enable_interrupts();
    return 0;
}

int kern_deschedule(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int *reject = (int *)esi;
    disable_interrupts();
    if (*reject != 0) {
        enable_interrupts();
        return 0;
    }
    sche_yield(SUSPENDED);
    enable_interrupts();
    return 0;
}

int kern_make_runnable(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    int tid = (int)esi;
    thread_t *thr = tcb_hashtab_get(tid);
    if (thr == NULL) return -1;
    if (thr->status != SUSPENDED) return -1;
    disable_interrupts();
    thr->status = RUNNABLE;
    sche_push_back(thr);
    enable_interrupts();
    return 0;
}

unsigned int kern_get_ticks(void) {
    return get_timer_ticks();
}

int kern_sleep(void) {
    int ticks = (int)asm_get_esi();

    if (ticks == 0) return 0;
    unsigned int cur_ticks = get_timer_ticks();
    unsigned int wakeup_ticks = cur_ticks + (unsigned int)ticks;
    if (wakeup_ticks < cur_ticks) return -1;

    sleep_node_t sleep_node;
    sleep_node.thread = get_cur_tcb();
    sleep_node.wakeup_ticks = wakeup_ticks;

    disable_interrupts();
    tranquilize(&sleep_node);
    sche_yield(SLEEPING);

    return 0;
}


