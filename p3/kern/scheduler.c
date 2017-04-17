/**
 * @file scheduler.c
 * @brief This file contains the functions that are used to switch context
 * @author Newton Xie (ncx)
 * @author Qiaoyu Deng (qdeng)
 * @bug No known bugs
 */

/* libc includes. */
#include <stdio.h>                    /* NULL */
#include <asm.h>                      /* disable_interrupts enable_interrupts */

/* x86 specific includes */
#include <x86/cr.h>                   /* set_cr3, set_esp0 */

/* DEBUG includes */
#include <simics.h>                   /* lprintf */

#include "scheduler.h"
#include "task.h"                     /* thread_t, task_t */
#include "asm_kern_to_user.h"         /* kern_to_user */
#include "asm_context_switch.h"       /* three switch functions */
#include "drivers/timer_driver.h"     /* get_num_ticks */
#include "utils/kern_mutex.h"         /* kern_mutex */

/* extern global variable  */
/* when there is no thread that can be switched to, we switch to idle thread */
extern thread_t *idle_thread;

/* static global variable */
static sche_node_t *cur_sche_node;     /* save the current running thread*/
static schedule_t sche_list;           /* scheduler uses FIFO list to switch */

/**
 * Initialize the scheduler's list structures.
 * @return 0 for success, -1 for failure
 */
int scheduler_init() {
    sche_list.active_list = list_init();
    if (sche_list.active_list == NULL) return -1;

    sche_list.sleeping_list = list_init();
    if (sche_list.sleeping_list == NULL) {
        list_destroy(sche_list.active_list);
        return -1;
    }

    return 0;
}

/**
 * Set current running threads
 * @param tcb_ptr pointer to thread control block structure
 */
void set_cur_run_thread(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    cur_sche_node = sche_node;
}

/**
 * Main function that is used to switch context.
 * @param status    control after the context switch, which status the thread
 *                  will be
 *
 * #define RUNNABLE 0
 * #define INITIALIZED 1
 * #define SUSPENDED 2
 * #define FORKED 3
 * #define BLOCKED_MUTEX 4
 * #define BLOCKED_WAIT 5
 * #define ZOMBIE 6
 * #define SLEEPING 7
 */
void sche_yield(int status) {
    /* we need disable interrupt to prevent context switch itself is switched */
    disable_interrupts();
    sche_node_t *new_sche_node;
    /*
    to check whether there is sleeping thread need to be waked up, otherwise we
    choose a thread from FIFO list to run.
     */
    sleep_node_t *sleeper;
    sleeper = (sleep_node_t *)get_first_node(sche_list.sleeping_list);
    /* check whether the ticks reaches the setting value of sleeping threads */
    if (sleeper != NULL && sleeper->wakeup_ticks <= get_timer_ticks()) {
        pop_first_node(sche_list.sleeping_list);
        sleeper->thread->status = RUNNABLE;
        new_sche_node = TCB_TO_SCHE_NODE(sleeper->thread);
    } else {
        /* get thread from normal FIFO list */
        new_sche_node = pop_first_node(sche_list.active_list);
    }

    thread_t *cur_tcb_ptr = SCHE_NODE_TO_TCB(cur_sche_node);
    cur_tcb_ptr->status = status;

    /* if there is thread can be switched to */
    if (new_sche_node != NULL) {
        /* if just doing context switch instead of blocking or sleeping */
        if (status == RUNNABLE && cur_tcb_ptr != idle_thread)
            add_node_to_tail(sche_list.active_list, cur_sche_node);

        thread_t *new_tcb_ptr = SCHE_NODE_TO_TCB(new_sche_node);
        cur_sche_node = new_sche_node;
        /* set new thread's kernel stack */
        set_esp0(new_tcb_ptr->kern_sp);

        /*
        status = RUNNABLE: Switch to a normal thread that will execute in the
            next CPU time slice.
        status = INITIALIZED: This status indicates the thread is initialized in
            kern_main, we need switch from kernel mode to user mode to run them
            by using iret when running them for the first time.
        status = FORKED: This status indicates the thread is in a newly forked
            task, we need set its eip explicitly into the fork function to let
            it return 0 (because it is a child task).
         */
        if (new_tcb_ptr->status == RUNNABLE) {
            /*
            we might switch to thread in different tasks, so we need change page
            directory pointer by set_cr3.
             */
            int original_task_id = cur_tcb_ptr->task->task_id;
            int new_task_id = new_tcb_ptr->task->task_id;
            if (new_task_id != original_task_id)
                set_cr3((uint32_t)new_tcb_ptr->task->page_dir);

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
    } else if (status != RUNNABLE) {
        /* otherwise wake up idle thread */
        cur_sche_node = TCB_TO_SCHE_NODE(idle_thread);
        /* set next interrupt kernel stack  */
        set_esp0(idle_thread->kern_sp);
        /* set page directory pointer */
        set_cr3((uint32_t)idle_thread->task->page_dir);
        int old_status = idle_thread->status;
        idle_thread->status = RUNNABLE;

        /* idle task is run for the first time */
        if (old_status == INITIALIZED) {
            asm_switch_to_initialized(&cur_tcb_ptr->cur_sp,
                                      idle_thread->cur_sp, idle_thread->ip);
        } else if (old_status == RUNNABLE) {
            asm_switch_to_runnable(&cur_tcb_ptr->cur_sp,
                                   idle_thread->cur_sp);
        } else {
            lprintf("how did i get here?");
            while (1) continue;
        }
    }

    enable_interrupts();
}

/**
 * Get current running thread's thread control block.
 * @return tcb pointer
 */
thread_t *get_cur_tcb() {
    return SCHE_NODE_TO_TCB(cur_sche_node);
}

/**
 * Put the thread control block to the tail of scheduler list
 * @param tcb_ptr the tcb that needs to be added back to list.
 */
void sche_push_back(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    add_node_to_tail(sche_list.active_list, sche_node);
}

/**
 * Put the thread control block to the head of scheduler list
 * @param tcb_ptr the tcb that needs to be added back to list.
 */
void sche_push_front(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    add_node_to_head(sche_list.active_list, sche_node);
}

/**
 * Move thread control block in the scheduler list to the head of list.
 * @param tcb_ptr the tcb that needs to be pushed the first one.
 */
void sche_move_front(thread_t *tcb_ptr) {
    sche_node_t *sche_node = TCB_TO_SCHE_NODE(tcb_ptr);
    remove_node(sche_list.active_list, sche_node);
    add_node_to_head(sche_list.active_list, sche_node);
}

/**
 * Insert a sleeping thread to sleeping list in order according to their
 * sleeping time(ticks). So our scheduler could only always check the first one
 * in the list to determine whether to wake it up instead of scanning th entire
 * list.
 * @param sleep_node list node that contains sleeping tcb pointer
 */
void tranquilize(sleep_node_t *sleep_node) {
    // TODO casting everywhere
    sleep_node_t *temp;
    node_t *next;
    temp = (sleep_node_t *)get_first_node(sche_list.sleeping_list);
    while (temp != NULL) {
        /* find the right place to insert */
        if (temp->wakeup_ticks <= sleep_node->wakeup_ticks) {
            next = get_next_node(sche_list.sleeping_list, (node_t *)temp);
            temp = (sleep_node_t *)next;
        } else {
            break;
        }
    }

    if (temp == NULL)
        add_node_to_tail(sche_list.sleeping_list, (node_t *)sleep_node);
    else
        insert_before(sche_list.sleeping_list, (node_t *)temp, (node_t *)sleep_node);
}
