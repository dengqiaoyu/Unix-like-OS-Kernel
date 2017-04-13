#include <stdio.h>      /* NULL */
#include <mutex.h>      /* mutex */
#include <asm.h>        /* disable_interrupts enable_interrupts */
#include <x86/cr.h>
#include <simics.h>     /* lprintf */
#include <interrupt_defines.h>

#include "scheduler.h"
#include "task.h"       /* thread_t */
#include "allocator.h"
#include "asm_switch.h" /* kern_to_user */
#include "asm_context_switch.h"

// DEBUG
#define print_line lprintf("line %d, tid: %d", __LINE__,\
                           SCHE_NODE_TO_TCB(cur_sche_node)->tid)
#define NUM_CHUNK_SCHEDULER 64

thread_t *idle_thread;

extern unsigned int num_ticks;

allocator_t *sche_allocator;
static sche_node_t *cur_sche_node;
static schedule_t sche_list;

int scheduler_init() {
    int ret = SUCCESS;
    ret = allocator_init(
              &sche_allocator,
              sizeof(sche_node_t) + sizeof(tcb_tb_node_t) +
              sizeof(thread_node_t) + sizeof(thread_t),
              NUM_CHUNK_SCHEDULER);
    if (ret != SUCCESS)
        return ret;

    sche_list.active_list = list_init();
    if (sche_list.active_list == NULL) return -1;

    sche_list.sleeping_list = list_init();
    if (sche_list.sleeping_list == NULL) {
        list_destroy(sche_list.active_list);
        return -1;
    }

    return ret;
}

void set_cur_run_thread(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    cur_sche_node = sche_node;
}

void sche_yield(int status) {
    disable_interrupts();

    // TODO where should this happen?
    outb(INT_ACK_CURRENT, INT_CTL_PORT);

    sche_node_t *new_sche_node;
    sleep_node_t *sleeper;
    sleeper = (sleep_node_t *)get_first_node(sche_list.sleeping_list);
    if (sleeper != NULL && sleeper->wakeup_ticks <= num_ticks) {
        pop_first_node(sche_list.sleeping_list);
        sleeper->thread->status = RUNNABLE;
        new_sche_node = TCB_TO_SCHE_NODE(sleeper->thread);
    }
    else {
        new_sche_node = pop_first_node(sche_list.active_list);
    }

    thread_t *cur_tcb_ptr = SCHE_NODE_TO_TCB(cur_sche_node);
    cur_tcb_ptr->status = status;

    if (new_sche_node != NULL) {
        if (status == RUNNABLE)
            add_node_to_tail(sche_list.active_list, cur_sche_node);

        thread_t *new_tcb_ptr = SCHE_NODE_TO_TCB(new_sche_node);
        cur_sche_node = new_sche_node;
        set_esp0(new_tcb_ptr->kern_sp);

        if (new_tcb_ptr->status == RUNNABLE) {
            int original_task_id = cur_tcb_ptr->task->task_id;
            int new_task_id = new_tcb_ptr->task->task_id;
            if (new_task_id != original_task_id)
                set_cr3((uint32_t)new_tcb_ptr->task->page_dir);

            asm_switch_to_runnable(&cur_tcb_ptr->cur_sp,
                                   new_tcb_ptr->cur_sp);
        }
        else if (new_tcb_ptr->status == INITIALIZED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;

            asm_switch_to_initialized(&cur_tcb_ptr->cur_sp,
                                      new_tcb_ptr->cur_sp, new_tcb_ptr->ip);
        }
        else if (new_tcb_ptr->status == FORKED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;

            asm_switch_to_forked(&cur_tcb_ptr->cur_sp,
                                 new_tcb_ptr->cur_sp,
                                 new_tcb_ptr->ip);
        }
    }
    else if (status != RUNNABLE) {
        cur_sche_node = TCB_TO_SCHE_NODE(idle_thread);
        set_esp0(idle_thread->kern_sp);

        set_cr3((uint32_t)idle_thread->task->page_dir);
        int old_status = idle_thread->status;
        idle_thread->status = RUNNABLE;

        if (old_status == INITIALIZED) {
            asm_switch_to_initialized(&cur_tcb_ptr->cur_sp,
                                      idle_thread->cur_sp, idle_thread->ip);
        }
        else if (old_status == RUNNABLE) {
            asm_switch_to_runnable(&cur_tcb_ptr->cur_sp,
                                   idle_thread->cur_sp);
        }
        else {
            lprintf("how did i get here?");
            while (1) continue;
        }
    }

    // TODO is it possible the thread initially had interrupts disabled?
    enable_interrupts();
}

thread_t *get_cur_tcb() {
    return SCHE_NODE_TO_TCB(cur_sche_node);
}

void sche_push_back(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    add_node_to_tail(sche_list.active_list, sche_node);
}

void sche_push_front(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    add_node_to_head(sche_list.active_list, sche_node);
}

void tranquilize(sleep_node_t *sleep_node) {
    // TODO casting everywhere

    sleep_node_t *temp;
    node_t *next;
    temp = (sleep_node_t *)get_first_node(sche_list.sleeping_list);
    while (temp != NULL) {
        if (temp->wakeup_ticks <= sleep_node->wakeup_ticks) {
            next = get_next_node(sche_list.sleeping_list, (node_t *)temp);
            temp = (sleep_node_t *)next;
        }
        else {
            break;
        }
    }

    if (temp == NULL)
        add_node_to_tail(sche_list.sleeping_list, (node_t *)sleep_node);
    else
        insert_before(sche_list.sleeping_list, next, (node_t *)sleep_node);
}
