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
#define print_line lprintf("line %d, tid: %d", __LINE__, GET_TCB_FROM_SCHE(cur_sche_node)->tid)
#define NUM_CHUNK_SCHEDULER 64

allocator_t *sche_allocator = NULL;
static sche_node_t *cur_sche_node = NULL;
static schedule_t sche_list = {{0}};

sche_node_t *_sche_node_pop_front();

int scheduler_init() {
    int ret = SUCCESS;
    ret = allocator_init(
              &sche_allocator,
              sizeof(sche_node_t) + sizeof(tcb_tb_node_t) + sizeof(thread_t),
              NUM_CHUNK_SCHEDULER);
    if (ret != SUCCESS)
        return ret;

    ret = list_init(&sche_list.active_list);
    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
}

void set_cur_run_thread(thread_t *tcb_ptr) {
    sche_node_t *sche_node = GET_SCHE_NODE_FROM_TCB(tcb_ptr);
    cur_sche_node = sche_node;
}

void sche_yield(int suspend_flag) {
    disable_interrupts();
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    thread_t *cur_tcb_ptr = GET_TCB_FROM_SCHE(cur_sche_node);
    sche_node_t *new_sche_node = _sche_node_pop_front();
    if (new_sche_node != NULL) {
        thread_t *new_tcb_ptr = GET_TCB_FROM_SCHE(new_sche_node);
        if (suspend_flag) cur_tcb_ptr->status = SUSPENDED;
        else sche_push_back(cur_tcb_ptr);
        cur_sche_node = new_sche_node;
        set_esp0(new_tcb_ptr->kern_sp);
        int original_task_id = cur_tcb_ptr->task->task_id;
        int new_task_id = new_tcb_ptr->task->task_id;
        if (new_tcb_ptr->status == RUNNABLE) {
            if (new_task_id != original_task_id) {
                set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            }
            // previous asm
            // __asm__("PUSHA");
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->cur_esp));
            // __asm__("movl %0, %%esp" :: "r" (new_tcb_ptr->cur_esp));
            // __asm__("POPA");
            // previous asm
            asm_switch_to_runnable(&cur_tcb_ptr->cur_esp,
                                   new_tcb_ptr->cur_esp);
        } else if (new_tcb_ptr->status == INITIALIZED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            // previous asm
            // __asm__("PUSHA");
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->cur_esp));
            // kern_to_user(new_tcb_ptr->user_sp, new_tcb_ptr->ip);
            // never reach here
            // previous asm
            asm_switch_to_initialized(&cur_tcb_ptr->cur_esp,
                                      new_tcb_ptr->user_sp, new_tcb_ptr->ip);
        } else if (new_tcb_ptr->status == FORKED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            // previous asm
            // __asm__("PUSHA");
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->cur_esp));
            // __asm__("movl %0, %%esp" :: "r" (new_tcb_ptr->cur_esp));
            // __asm__("jmp %0" :: "r" (new_tcb_ptr->ip));
            // never reach here
            // previous asm
            asm_switch_to_forked(&cur_tcb_ptr->cur_esp,
                                 new_tcb_ptr->cur_esp,
                                 new_tcb_ptr->ip);
        }
    }
    // TODO
    // is it possible the thread initially had interrupts disabled?
    enable_interrupts();
}

thread_t *get_cur_tcb() {
    return GET_TCB_FROM_SCHE(cur_sche_node);
}

void sche_push_back(thread_t *tcb_ptr) {
    sche_node_t *sche_node = GET_SCHE_NODE_FROM_TCB(tcb_ptr);
    add_node_to_tail(&sche_list.active_list, sche_node);
}

void sche_push_front(thread_t *tcb_ptr) {
    sche_node_t *sche_node = GET_SCHE_NODE_FROM_TCB(tcb_ptr);
    add_node_to_head(&sche_list.active_list, sche_node);
}

sche_node_t *_sche_node_pop_front() {
    return pop_first_node(&sche_list.active_list);
}
