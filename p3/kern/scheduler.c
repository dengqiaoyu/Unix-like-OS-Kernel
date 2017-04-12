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

allocator_t *sche_allocator;
static sche_node_t *cur_sche_node;
static schedule_t sche_list;

sche_node_t *_sche_node_pop_front();

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

    return ret;
}

void set_cur_run_thread(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    cur_sche_node = sche_node;
}

void sche_yield(int status) {
    disable_interrupts();
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    thread_t *cur_tcb_ptr = SCHE_NODE_TO_TCB(cur_sche_node);
    sche_node_t *new_sche_node = _sche_node_pop_front();
    if (new_sche_node != NULL) {
        thread_t *new_tcb_ptr = SCHE_NODE_TO_TCB(new_sche_node);

        if (status == RUNNABLE) {
            add_node_to_tail(sche_list.active_list, cur_sche_node);
        } else cur_tcb_ptr->status = status;
        cur_sche_node = new_sche_node;
        set_esp0(new_tcb_ptr->kern_sp);
        int original_task_id = cur_tcb_ptr->task->task_id;
        int new_task_id = new_tcb_ptr->task->task_id;
        if (new_tcb_ptr->status == RUNNABLE) {
            if (new_task_id != original_task_id) {
                set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            }
            asm_switch_to_runnable(&cur_tcb_ptr->cur_sp,
                                   new_tcb_ptr->cur_sp);
        } else if (new_tcb_ptr->status == INITIALIZED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            asm_switch_to_initialized(&cur_tcb_ptr->cur_sp,
                                      new_tcb_ptr->cur_sp, new_tcb_ptr->ip);
        } else if (new_tcb_ptr->status == FORKED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            asm_switch_to_forked(&cur_tcb_ptr->cur_sp,
                                 new_tcb_ptr->cur_sp,
                                 new_tcb_ptr->ip);
        }
    }
    // TODO
    // is it possible the thread initially had interrupts disabled?
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

sche_node_t *_sche_node_pop_front() {
    return pop_first_node(sche_list.active_list);
}
