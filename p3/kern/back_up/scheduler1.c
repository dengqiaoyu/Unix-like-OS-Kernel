#include <stdio.h>      /* NULL */
#include <mutex.h>      /* mutex */
#include <asm.h>        /* disable_interrupts enable_interrupts */
#include <x86/cr.h>
#include <simics.h>     /* lprintf */
#include <interrupt_defines.h>

#include "scheduler.h"
#include "task.h"       /* thread_t */
#include "task_internal.h"
#include "allocator.h"
#include "asm_switch.h" /* kern_to_user */
#include "asm_context_switch.h"

// DEBUG
#define print_line lprintf("line %d, tid: %d", __LINE__, GET_TCB(cur_sche_node)->tid)
#define NUM_CHUNK_SCHEDULER 64

allocator_t *sche_allocator = NULL;
sche_node_t *cur_sche_node = NULL;
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
    if (sche_list.active_list == NULL) {
        // TODO destroy allocator
        return -1;
    }

    sche_list.deactive_list = list_init();
    if (sche_list.deactive_list == NULL) {
        // TODO destroy allocator
        list_destroy(sche_list.active_list);
        return -1;
    }

    ret = mutex_init(&sche_list.sche_list_mutex);
    if (ret != SUCCESS) {
        // TODO destroy allocator
        list_destroy(sche_list.deactive_list);
        list_destroy(sche_list.active_list);
        return ret;
    }

    return SUCCESS;
}

void append_to_scheduler(sche_node_t *sche_node) {
    mutex_lock(&sche_list.sche_list_mutex);
    add_node_to_tail(sche_list.active_list, sche_node);
    mutex_unlock(&sche_list.sche_list_mutex);
}

sche_node_t *pop_scheduler_active() {
    mutex_lock(&sche_list.sche_list_mutex);
    sche_node_t *sche_node = pop_first_node(sche_list.active_list);
    mutex_unlock(&sche_list.sche_list_mutex);
    return sche_node;
}

void sche_yield() {
    disable_interrupts();
    outb(INT_ACK_CURRENT, INT_CTL_PORT);
    thread_t *cur_tcb_ptr = GET_TCB(cur_sche_node);
    sche_node_t *new_sche_node = pop_scheduler_active();
    if (new_sche_node != NULL) {
        thread_t *new_tcb_ptr = GET_TCB(new_sche_node);
        append_to_scheduler(cur_sche_node);
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
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->curr_esp));
            // __asm__("movl %0, %%esp" :: "r" (new_tcb_ptr->curr_esp));
            // __asm__("POPA");
            // previous asm
            asm_switch_to_runnable(&cur_tcb_ptr->cur_sp,
                                   new_tcb_ptr->cur_sp);
        } else if (new_tcb_ptr->status == INITIALIZED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            // previous asm
            // __asm__("PUSHA");
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->curr_esp));
            // kern_to_user(new_tcb_ptr->user_sp, new_tcb_ptr->ip);
            // never reach here
            // previous asm
            asm_switch_to_initialized(&cur_tcb_ptr->cur_sp,
                                      new_tcb_ptr->cur_sp, new_tcb_ptr->ip);
        } else if (new_tcb_ptr->status == FORKED) {
            set_cr3((uint32_t)new_tcb_ptr->task->page_dir);
            new_tcb_ptr->status = RUNNABLE;
            // previous asm
            // __asm__("PUSHA");
            // __asm__("movl %%esp, %0" : "=r" (cur_tcb_ptr->curr_esp));
            // __asm__("movl %0, %%esp" :: "r" (new_tcb_ptr->curr_esp));
            // __asm__("jmp %0" :: "r" (new_tcb_ptr->ip));
            // never reach here
            // previous asm
            asm_switch_to_forked(&cur_tcb_ptr->cur_sp,
                                 new_tcb_ptr->cur_sp,
                                 new_tcb_ptr->ip);
        }
    }
    // TODO
    // is it possible the thread initially had interrupts disabled?
    enable_interrupts();
}
