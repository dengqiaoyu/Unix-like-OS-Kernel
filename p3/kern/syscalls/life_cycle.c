/**
 * @file    life_cycle.c
 * @brief   This file contains the system calls that are used for controlling
 *          system and tasks' life cycle.
 * @author  Newton Xie (ncx)
 * @author  Qiaoyu Deng (qdeng)
 * @bug     No known bugs
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cr.h>                  /* set_crX */
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */
#include <console.h>             /* clear_console */
#include <simics.h>              /* lprintf and debug */

/* user define includes */
#include "syscalls/syscalls.h"
#include "utils/kern_mutex.h"
#include "task.h"                /* task thread declaration and interface */
#include "scheduler.h"           /* scheduler declaration and interface */
#include "vm.h"                  /* virtual memory management */
#include "asm_kern_to_user.h"    /* asm_kern_to_user */

#define EXECNAME_MAX 64
#define ARGVEC_MAX 128
#define ARGC_MAX 16
#define N_MAIN_ARGS 4

/* helper function declaration */
void asm_set_exec_context(uint32_t old_kern_sp,
                          uint32_t new_kern_sp,
                          uint32_t *new_cur_sp_ptr,
                          uint32_t *new_ip_ptr);
void asm_hlt(void);

/**
 * @brief   Creates a new task.
 * The new task receives an exact, coherent copy of all memory regions of the
 * invoking task. The new task contains a single thread which is a copy of the
 * thread invoking fork() except for the return value of the system call.
 * If fork() succeeds, the invoking thread will receive the ID of the new task’s
 * thread and the newly created thread will receive the value zero. The exit
 * status of a newly-created task is 0. If a thread in the task invoking fork()
 * has a software exception handler registered, the corresponding thread in the
 * newly-created task will have exactly the same handler registered.
 * @return  -1 if task has one more threads, or fork fails to get new memory.
 *          return 0 if child task get created successfully, child's tid if
 *          parent task create child task successfully.
 */
int kern_fork(void) {
    int ret = 0;
    thread_t *old_thread = get_cur_tcb();
    /* get parent task information */
    int old_tid = old_thread->tid;
    task_t *old_task = old_thread->task;
    thread_t *cur_thr = NULL;

    /* we need to check whether task has only one thread */
    kern_mutex_lock(&(old_task->thread_list_mutex));
    int live_threads = get_list_size(old_task->live_thread_list);
    kern_mutex_unlock(&(old_task->thread_list_mutex));
    if (live_threads > 1) return -1;

    /* get essential task control block and assign important resources */
    task_t *new_task = task_init();
    if (new_task == NULL) {
        lprintf("task_init() failed in kern_fork at line %d", __LINE__);
        return -1;
    }
    new_task->parent_task = old_task;

    /* copy parent's page directory to the child */
    ret = page_dir_copy(new_task->page_dir, old_task->page_dir);
    if (ret != 0) {
        lprintf("page_dir_copy() failed in kern_fork at line %d", __LINE__);
        task_destroy(new_task);
        return -1;
    }

    /* copy parent's memory mapping to child */
    ret = maps_copy(old_task->maps, new_task->maps);
    if (ret != 0) {
        lprintf("maps_copy() failed in kern_fork at line %d", __LINE__);
        task_destroy(new_task);
        return -1;
    }

    /* get essential thread control block and assign important resources */
    thread_t *new_thread = thread_init();
    if (new_thread == NULL) {
        lprintf("thread_init() failed in kern_fork at line %d", __LINE__);
        task_destroy(new_task);
        return -1;
    }

    new_task->task_id = new_thread->tid;
    new_thread->task = new_task;
    /**
     * the scheduler need to tell the difference between FORKED and RUNNABLE to
     * determine which way to set the registers.
     */
    new_thread->status = FORKED;
    /* get the same swexn handler just like parent */
    new_thread->swexn_sp = old_thread->swexn_sp;
    new_thread->swexn_handler = old_thread->swexn_handler;
    new_thread->swexn_arg = old_thread->swexn_arg;
    /* add new thread to new task's thread list */
    add_node_to_head(new_task->live_thread_list, TCB_TO_LIST_NODE(new_thread));
    /* add new task to parent's child task list */
    kern_mutex_lock(&(old_task->child_task_list_mutex));
    add_node_to_head(old_task->child_task_list, TASK_TO_LIST_NODE(new_task));
    kern_mutex_unlock(&(old_task->child_task_list_mutex));
    /* copy parent's execution context to child */
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         &(new_thread->cur_sp),
                         &(new_thread->ip));

    /* Now, we will have two tasks running */
    /* Cannot declare variables here, because we will break the stack */
    cur_thr = get_cur_tcb();
    if (cur_thr->tid != old_tid) {
        return 0;
    } else {
        disable_interrupts();
        sche_push_back(new_thread);
        enable_interrupts();
        return new_thread->tid;
    }
}

/**
 * @brief Creates a new thread. The rest of code is the same like fork.
 * @return  new thread's tid when succeeds, otherwise returns -1
 */
int kern_thread_fork(void) {
    thread_t *old_thread = get_cur_tcb();
    task_t *cur_task = old_thread->task;
    int old_tid = old_thread->tid;
    thread_t *cur_thr = NULL;

    thread_t *new_thread = thread_init();
    if (new_thread == NULL) {
        lprintf("thread_init() failed in kern_thread_fork at line %d",
                __LINE__);
        return -1;
    }

    kern_mutex_lock(&cur_task->thread_list_mutex);
    add_node_to_tail(cur_task->live_thread_list, TCB_TO_LIST_NODE(new_thread));
    kern_mutex_unlock(&cur_task->thread_list_mutex);

    new_thread->task = cur_task;
    new_thread->status = FORKED;
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         &(new_thread->cur_sp),
                         &(new_thread->ip));

    cur_thr = get_cur_tcb();
    if (cur_thr->tid != old_tid) {
        return 0;
    } else {
        disable_interrupts();
        sche_push_back(new_thread);
        enable_interrupts();
        return new_thread->tid;
    }
}

/** @brief  Replaces the currently running program with another.
 *  
 *  If the calling task has multiple threads, an error is returned.
 *  The total length of argvec arguments cannot exceed 128, and the number
 *  of arguments cannot exceed 16. If all arguments are within length bounds
 *  and in valid user memory, then a new stack is setup with four arguments:
 *  argc, argv, and high and low bounds for the initial user stack.
 *
 *  @return no return on success and negative on failure
 */
int kern_exec(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    char *execname = (char *)(*esi);
    char **argvec = (char **)(*(esi + 1));

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    kern_mutex_lock(&(task->thread_list_mutex));
    int live_threads = get_list_size(task->live_thread_list);
    kern_mutex_unlock(&(task->thread_list_mutex));
    if (live_threads > 1) return -1;

    char **temp = argvec;
    int argc = 0;
    int total_len = 0;
    int ret;
    int len;

    /* check whether temp points to a valid location in user meory */
    ret = validate_user_mem((uint32_t)temp, sizeof(char *), MAP_USER);
    /* if temp is NULL, argv is empty and we can move on */
    if (temp && ret < 0) return -1;

    while (temp && *temp) {
        char *arg = *temp;
        /* validate_user_string return string length if valid */
        len = validate_user_string((uint32_t)arg, ARGVEC_MAX);
        if (len <= 0) return -1;
        total_len += len;
        if (total_len > ARGVEC_MAX) return -1;

        argc += 1;
        if (argc > ARGC_MAX) return -1;

        /* increment temp, which points to the next argument */
        temp += 1;
        ret = validate_user_mem((uint32_t)temp, sizeof(char *), MAP_USER);
        if (ret < 0) return -1;
    }

    ret = validate_user_string((uint32_t)execname, EXECNAME_MAX);
    if (ret <= 0) return -1;
    // copy execname to kernel memory so we can clear user memory
    char namebuf[EXECNAME_MAX];
    sprintf(namebuf, "%s", execname);

    simple_elf_t elf_header;
    ret = elf_load_helper(&elf_header, namebuf);
    if (ret < 0) return -1;

    // copy arguments to kernel memory so we can clear user memory
    char argbuf[ARGVEC_MAX];
    char *ptrbuf[ARGC_MAX];
    char *arg;
    char *marker = argbuf;

    int i;
    for (i = 0; i < argc; i++) {
        // point arg to next argument
        arg = *(argvec + i);
        // we can use strlen now since we know that the argument terminates
        len = strlen(arg);
        strncpy(marker, arg, len);
        marker[len] = '\0';
        // store the argument's kernel memory address
        ptrbuf[i] = marker;
        // point marker to the next free space in argbuf
        marker += len + 1;
    }

    thread->cur_sp = USER_STACK_START;
    thread->ip = elf_header.e_entry;
    // we need to deregister the swexn handler if one exists
    thread->swexn_sp = NULL;
    thread->swexn_handler = NULL;
    thread->swexn_arg = NULL;

    /*
     * THERE IS A KNOWN BUG HERE.
     *
     * At this point we start clearing out the user memory and setting up
     * a clean execution state for the new program. However, we could still
     * fail when setting the new map list or loading the new elf binary.
     * In this case, we can no longer return to the calling code, and must
     * vanish...
     */

    maps_clear(task->maps);
    // reserve the kernel memory region
    ret = maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES - 1, 0);
    if (ret < 0) {
        lprintf("i commited to exec and failed :(");
        kern_vanish();
    }
    // reserve the highest page for physical reads and writes
    ret = maps_insert(task->maps, RW_PHYS_VA, RW_PHYS_VA + PAGE_SIZE - 1, 0);
    if (ret < 0) {
        lprintf("i commited to exec and failed :(");
        kern_vanish();
    }

    page_dir_clear(task->page_dir);
    set_cr3((uint32_t)task->page_dir);

    // load the new binary into the fresh virtual memory
    ret = load_program(&elf_header, task->maps);
    if (ret < 0) {
        lprintf("i commited to exec and failed :(");
        kern_vanish();
    }

    /* point argv to immediately above the arguments on the main stack */
    char **argv = (char **)USER_STACK_START;
    argv += N_MAIN_ARGS + 1; // plus one for the return address

    /* point buf to immediately above the argument vector */
    char *buf = (char *)argv;
    buf += (argc + 1) * sizeof(int); // plus one for the argv null terminator

    // copy from kernel memory to user space
    for (i = 0; i < argc; i++) {
        arg = *(ptrbuf + i);
        len = strlen(arg);
        strncpy(buf, arg, len);
        buf[len] = '\0';
        argv[i] = buf;
        buf += len + 1;
    }
    // null terminate the user argument vector
    argv[argc] = NULL;

    uint32_t *ptr = (uint32_t *)USER_STACK_START;
    *(ptr + 1) = argc;
    *(ptr + 2) = (uint32_t)argv;
    *(ptr + 3) = USER_STACK_LOW + USER_STACK_SIZE;
    *(ptr + 4) = USER_STACK_LOW;

    // update fname for simics symbolic debugging
    sim_reg_process(task->page_dir, elf_header.e_fname);

    set_esp0(thread->kern_sp);
    kern_to_user(USER_STACK_START, elf_header.e_entry);

    return 0;
}

/**
 * Sets the exit status of the current task to status.
 */
void kern_set_status(void) {
    int status = (int)asm_get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task->status = status;
}

/** @brief  Causes a thread to cease execution.
 *
 *  The thread moves itself from its task's live thread list to the zombie
 *  thread list.
 *
 *  If the thread is the last in its task, it does some work:
 *
 *      1:  It "orphans" child tasks (dead or alive) to the init task.
 *      2:  It cleans up virtual memory resources.
 *      3:  It removes the task from its parent's child task list.
 *      4a: If the parent has waiting threads, it wakes a waiting thread, which
 *          will receive the task's id and status. If the task is the last child
 *          of its parent, then all the parent's waiting threads are woken.
 *       b: Otherwise, it adds the task to its parent's zombie task list, where
 *          it can be waited on. If the zombie task list is populated, it frees
 *          as many resources as possible from the previous zombie task.
 *
 *  The thread then yields and will never be scheduled again.
 */
void kern_vanish(void) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    kern_mutex_lock(&(task->thread_list_mutex));
    remove_node(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    int live_threads = get_list_size(task->live_thread_list);
    add_node_to_tail(task->zombie_thread_list, TCB_TO_LIST_NODE(thread));

    /*
     * We are missing an optimization here. We could free thread resources
     * from the previous zombie thread...
     */

    if (live_threads > 0) {
        /*
         * Need to disable interrupts here, before unlocking the thread list
         * mutex. Otherwise, another thread might be switched to and vanish,
         * finding that it is the last thread to vanish. Then the task could
         * be given to a waiting thread and destroyed while we are running...
         */
        disable_interrupts();
        cli_kern_mutex_unlock(&(task->thread_list_mutex));
        sche_yield(ZOMBIE);
    } else {
        kern_mutex_unlock(&(task->thread_list_mutex));

        orphan_children(task);
        orphan_zombies(task);
        page_dir_clear(task->page_dir);
        maps_clear(task->maps);

        // lock the vanish mutex to access the parent pointer
        kern_mutex_lock(&(task->vanish_mutex));
        task_t *parent = task->parent_task;
        if (parent == NULL) {
            panic("init or idle task vanished?");
        }

        // assumes we are in the parent's child task list
        kern_mutex_lock(&(parent->child_task_list_mutex));
        remove_node(parent->child_task_list, TASK_TO_LIST_NODE(task));
        int num_siblings = get_list_size(parent->child_task_list);
        kern_mutex_unlock(&(parent->child_task_list_mutex));

        // gain access to the parent's waiting threads and zombie task lists
        kern_mutex_lock(&(parent->wait_mutex));

        if (get_list_size(parent->waiting_thread_list) > 0) {
            node_t *node = pop_first_node(parent->waiting_thread_list);

            // get the pointer to the wait node on the waiting thread's stack
            wait_node_t *waiter = (wait_node_t *)node;
            waiter->zombie = task;

            // disable interrupts to protect scheduler structures
            disable_interrupts();

            // wake the waiting thread
            waiter->thread->status = RUNNABLE;
            sche_push_back(waiter->thread);

            // if the parent has no other children, we have to wake all waiters
            if (num_siblings == 0) {
                node = pop_first_node(parent->waiting_thread_list);
                while (node) {
                    waiter = (wait_node_t *)node;
                    waiter->thread->status = RUNNABLE;
                    sche_push_back(waiter->thread);
                    node = pop_first_node(parent->waiting_thread_list);
                }
            }

            cli_kern_mutex_unlock(&(parent->wait_mutex));
            cli_kern_mutex_unlock(&(task->vanish_mutex));

            sche_yield(ZOMBIE);
        } else {
            node_t *last_node = get_last_node(parent->zombie_task_list);
            if (last_node != NULL) {
                // we can free the previous zombie's memory and thread resources
                task_t *prev_zombie = LIST_NODE_TO_TASK(last_node);
                task_clear(prev_zombie);
            }

            /*
             * We must disable interrupts before adding the task to the zombie
             * task list. Otherwise, a waiting thread could receive the task
             * and destroy it before we finish yielding...
             */
            disable_interrupts();
            add_node_to_tail(parent->zombie_task_list, TASK_TO_LIST_NODE(task));
            cli_kern_mutex_unlock(&(parent->wait_mutex));
            cli_kern_mutex_unlock(&(task->vanish_mutex));
            sche_yield(ZOMBIE);
        }
    }
}

/** @brief  Collects the exit status of a child task.
 *
 *  The task's zombie task list is checked. If it is populated, then we simply
 *  pop the first zombie off of the list, free its resources, and return. If
 *  the zombie task list and child task list are both empty, then we return
 *  a failure.
 *
 *  Otherwise, we must block. We do so by allocating space on our stack for a
 *  "wait node", defined in task.h. This wait node is simply a linked list node
 *  with a pointer to its waiting thread and space for a task pointer. This
 *  wait node is added to the task's waiting thread list, and the caller yields.
 *
 *  When the caller is woken up, the zombie field of the wait node will be
 *  populated with a pointer to the task which has been received, or NULL if
 *  there are no more children.
 *
 *  @return zombie task id on success and negative on failure
 */
int kern_wait(void) {
    int *status_ptr = (int *)asm_get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    if (status_ptr != NULL) {
        int perms = MAP_USER | MAP_WRITE;
        int ret = validate_user_mem((uint32_t)status_ptr, sizeof(int), perms);
        if (ret < 0) return -1;
    }

    task_t *zombie;

    // get access to the waiting threads and zombie task lists
    kern_mutex_lock(&(task->wait_mutex));
    if (get_list_size(task->zombie_task_list) > 0) {
        zombie = LIST_NODE_TO_TASK(pop_first_node(task->zombie_task_list));
        kern_mutex_unlock(&(task->wait_mutex));
    } else {
        kern_mutex_lock(&(task->child_task_list_mutex));
        int active_children = get_list_size(task->child_task_list);
        kern_mutex_unlock(&(task->child_task_list_mutex));

        // return failure if no zombies or children
        if (active_children == 0) {
            kern_mutex_unlock(&(task->wait_mutex));
            return -1;
        }

        // declare a wait node on the stack
        wait_node_t wait_node;
        wait_node.thread = thread;

        disable_interrupts();

        /*
         * We must disable interrupts here before adding ourselves to the
         * waiting thread list. Otherwise, we could be "woken up" before
         * we block...
         */
        add_node_to_tail(task->waiting_thread_list, &(wait_node.node));
        cli_kern_mutex_unlock(&(task->wait_mutex));
        sche_yield(BLOCKED_WAIT);

        if (wait_node.zombie == NULL) return -1;
        else zombie = wait_node.zombie;
    }

    int ret = zombie->task_id;
    if (status_ptr != NULL) *status_ptr = zombie->status;

    task_destroy(zombie);
    return ret;
}

/**
 * Ceases execution of the operating system.
 */
void kern_halt(void) {
    clear_console();
    disable_interrupts();
    printf("Shutdown, Goodbye World!\n");
    sim_halt();
    asm_hlt();
    while (1)
        continue;
}
