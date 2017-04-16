#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <page.h>                /* PAGE_SIZE */
#include <cr.h>                  /* set_crX */
#include <asm.h>                 /* disable_interrupts(), enable_interrupts() */

#include <simics.h>              /* lprintf and debug */

/* user define includes */
#include "syscalls/syscalls.h"
#include "utils/mutex.h"
#include "task.h"                /* task thread declaration and interface */
#include "scheduler.h"           /* scheduler declaration and interface */
#include "vm.h"                  /* virtual memory management */
#include "asm_kern_to_user.h"    /* asm_kern_to_user */

/* helper function declaration */
void asm_set_exec_context(uint32_t old_kern_sp,
                          uint32_t new_kern_sp,
                          uint32_t *new_cur_sp_ptr,
                          uint32_t *new_ip_ptr);
void asm_hlt(void);

int kern_fork(void) {
    int ret = 0;
    thread_t *old_thread = get_cur_tcb();
    int old_tid = old_thread->tid;
    task_t *old_task = old_thread->task;
    thread_t *cur_thr = NULL;

    mutex_lock(&(old_task->thread_list_mutex));
    int live_threads = get_list_size(old_task->live_thread_list);
    mutex_unlock(&(old_task->thread_list_mutex));
    if (live_threads > 1) return -1;

    task_t *new_task = task_init();
    if (new_task == NULL) {
        lprintf("task_init() failed in kern_fork at line %d", __LINE__);
        return -1;
    }
    new_task->parent_task = old_task;

    ret = page_dir_copy(new_task->page_dir, old_task->page_dir);
    if (ret != 0) {
        lprintf("page_dir_copy() failed in kern_fork at line %d", __LINE__);
        undo_task_init(new_task);
        return -1;
    }

    ret = maps_copy(old_task->maps, new_task->maps);
    if (ret != 0) {
        lprintf("maps_copy() failed in kern_fork at line %d", __LINE__);
        undo_page_dir_copy(new_task->page_dir);
        undo_task_init(new_task);
        return -1;
    }

    thread_t *new_thread = thread_init();
    if (new_thread == NULL) {
        lprintf("thread_init() failed in kern_fork at line %d", __LINE__);
        undo_maps_copy(old_task->maps);
        undo_page_dir_copy(new_task->page_dir);
        undo_task_init(new_task);
        return -1;
    }
    new_task->task_id = new_thread->tid;
    new_thread->task = new_task;
    new_thread->status = FORKED;
    add_node_to_head(new_task->live_thread_list, TCB_TO_LIST_NODE(new_thread));

    mutex_lock(&(old_task->child_task_list_mutex));
    add_node_to_head(old_task->child_task_list, TASK_TO_LIST_NODE(new_task));
    mutex_unlock(&(old_task->child_task_list_mutex));
    asm_set_exec_context(old_thread->kern_sp,
                         new_thread->kern_sp,
                         &(new_thread->cur_sp),
                         &(new_thread->ip));
    // Now, we will have two tasks running
    // BUG that has been found!!! cannot declare var here, because we will break
    // stack
    // Do we need a mutex to protect this one?
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

int kern_thread_fork(void) {
    thread_t *old_thread = get_cur_tcb();
    task_t *cur_task = old_thread->task;
    thread_t *new_thread = thread_init();
    int old_tid = old_thread->tid;
    thread_t *cur_thr = NULL;
    if (new_thread == NULL) {
        lprintf("thread_init() failed in kern_thread_fork at line %d",
                __LINE__);
        return -1;
    }
    mutex_lock(&cur_task->thread_list_mutex);
    add_node_to_head(cur_task->live_thread_list, TCB_TO_LIST_NODE(new_thread));
    mutex_unlock(&cur_task->thread_list_mutex);
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

int kern_exec(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    char *execname = (char *)(*esi);
    char **argvec = (char **)(*(esi + 1));

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    mutex_lock(&(task->thread_list_mutex));
    int live_threads = get_list_size(task->live_thread_list);
    mutex_unlock(&(task->thread_list_mutex));
    if (live_threads > 1) return -1;

    char **temp = argvec;
    int argc = 0;
    int total_len = 0;
    int ret;
    int len;

    ret = validate_user_mem((uint32_t)temp, sizeof(char *), MAP_USER);
    if (temp && ret < 0) return -1;

    // why temp?
    // check parameters
    while (temp && *temp) {
        char *arg = *temp;
        len = validate_user_string((uint32_t)arg, 128);
        if (len <= 0) return -1;
        total_len += len;
        if (total_len > 128) return -1;

        argc += 1;
        if (argc > 16) return -1;
        temp += 1;

        ret = validate_user_mem((uint32_t)temp, sizeof(char *), MAP_USER);
        if (ret < 0) return -1;
    }


    ret = validate_user_string((uint32_t)execname, 64);
    if (ret <= 0) return -1;
    char namebuf[64];
    sprintf(namebuf, "%s", execname);

    lprintf("I am thread %d, I am going to execute %s", thread->tid, namebuf );
    simple_elf_t elf_header;
    ret = elf_load_helper(&elf_header, namebuf);
    if (ret < 0) return -1;

    char argbuf[128];
    char *ptrbuf[16];
    char *arg;
    char *marker = argbuf;

    int i;
    for (i = 0; i < argc; i++) {
        arg = *(argvec + i);
        len = strlen(arg);
        strncpy(marker, arg, len);
        marker[len] = '\0';
        ptrbuf[i] = marker;
        marker += len + 1;
    }

    thread->cur_sp = USER_STACK_START;
    thread->ip = elf_header.e_entry;

    maps_clear(task->maps);
    // kernel memory region
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES - 1, 0);
    // populated memory region for accessing physical frames
    maps_insert(task->maps, RW_PHYS_VA, RW_PHYS_VA + PAGE_SIZE - 1, 0);

    page_dir_clear(task->page_dir);
    set_cr3((uint32_t)task->page_dir);

    ret = load_program(&elf_header, task->maps);
    if (ret < 0) return -1;

    // need macros here badly
    // 5 because ret addr and 4 args
    char **argv = (char **)(USER_STACK_START + 5 * sizeof(int));
    // 6 because argv is a null terminated char ptr array
    char *buf = (char *)(USER_STACK_START + 6 * sizeof(int) +
                         argc * sizeof(int));

    for (i = 0; i < argc; i++) {
        arg = *(ptrbuf + i);
        len = strlen(arg);
        strncpy(buf, arg, len);
        buf[len] = '\0';
        argv[i] = buf;
        buf += len + 1;
    }
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

void kern_set_status(void) {
    int status = (int)asm_get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task->status = status;
}

void kern_vanish(void) {
    thread_t *thread = get_cur_tcb();
    lprintf("I am thread %d, I am going to vanish", thread->tid);
    task_t *task = thread->task;

    mutex_lock(&(task->vanish_mutex));
    task_t *parent = task->parent_task;

    mutex_lock(&(task->thread_list_mutex));
    remove_node(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    int live_threads = get_list_size(task->live_thread_list);
    add_node_to_tail(task->zombie_thread_list, TCB_TO_LIST_NODE(thread));
    // TODO try to free previous zombie stack
    lprintf("I am thread %d in line %d", thread->tid, __LINE__);
    if (live_threads > 0) {
        /*
         * need to disable interrupts here, before unlocking the thread list
         * mutex. otherwise, another thread might be switched to and vanish,
         * finding that it is the last thread to vanish. then the task could
         * be given to a waiting thread and destroyed, and with it our stack...
         */
        disable_interrupts();
        cli_mutex_unlock(&(task->thread_list_mutex));
        cli_mutex_unlock(&(task->vanish_mutex));
        sche_yield(ZOMBIE);
    } else {
        lprintf("I am thread %d in line %d", thread->tid, __LINE__);
        mutex_unlock(&(task->thread_list_mutex));

        orphan_children(task);
        orphan_zombies(task);

        lprintf("I am thread %d in line %d", thread->tid, __LINE__);
        if (parent == NULL) lprintf("init or idle task vanished?");
        mutex_lock(&(parent->wait_mutex));

        mutex_lock(&(parent->child_task_list_mutex));
        // assumes we are in the parent's child task list
        remove_node(parent->child_task_list, TASK_TO_LIST_NODE(task));
        mutex_unlock(&(parent->child_task_list_mutex));

        if (get_list_size(parent->waiting_thread_list) > 0) {
            lprintf("I am thread %d in line %d", thread->tid, __LINE__);
            node_t *node = pop_first_node(parent->waiting_thread_list);
            mutex_unlock(&(parent->wait_mutex));

            wait_node_t *waiter = (wait_node_t *)node;
            waiter->zombie = task;

            // must disable interrupts here
            // otherwise, the waiting thread might free us before we yield
            disable_interrupts();
            waiter->thread->status = RUNNABLE;
            lprintf("wake thread %d up", waiter->thread->tid);
            sche_push_back(waiter->thread);

            cli_mutex_unlock(&(task->vanish_mutex));
            lprintf("I am thread %d in line %d", thread->tid, __LINE__);
            sche_yield(ZOMBIE);
            lprintf("I am thread %d in line %d", thread->tid, __LINE__);
        } else {
            lprintf("I am thread %d in line %d", thread->tid, __LINE__);
            node_t *last_node = get_last_node(parent->zombie_task_list);
            if (last_node != NULL) {
                // we can free the previous zombie's vm and thread resources
                task_t *prev_zombie = LIST_NODE_TO_TASK(last_node);
                task_clear(prev_zombie);
            }

            // must disable interrupts here
            // otherwise, a waiting thread might free us before we yield
            disable_interrupts();
            add_node_to_tail(parent->zombie_task_list, TASK_TO_LIST_NODE(task));
            cli_mutex_unlock(&(parent->wait_mutex));

            cli_mutex_unlock(&(task->vanish_mutex));
            sche_yield(ZOMBIE);
            lprintf("I am thread %d in line %d", thread->tid, __LINE__);
        }
    }

    // TODO error handling
    lprintf("returned from end of vanish");
}

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
    mutex_lock(&(task->wait_mutex));
    if (get_list_size(task->zombie_task_list) > 0) {
        zombie = LIST_NODE_TO_TASK(pop_first_node(task->zombie_task_list));
        mutex_unlock(&(task->wait_mutex));
    } else {
        mutex_lock(&(task->child_task_list_mutex));
        int active_children = get_list_size(task->child_task_list);
        mutex_unlock(&(task->child_task_list_mutex));

        // another fork could happen in between, but that's fine
        if (active_children == 0) {
            mutex_unlock(&(task->wait_mutex));
            return -1;
        }

        // declare on stack
        wait_node_t wait_node;
        wait_node.thread = thread;

        disable_interrupts();

        // this unlock goes after disable_interrupts
        // otherwise, a child could turn into a zombie before blocking
        // then we might incorrectly block forever
        cli_mutex_unlock(&(task->wait_mutex));
        add_node_to_tail(task->waiting_thread_list, &(wait_node.node));
        lprintf("I am thread %d,  I am going to blocked wait", thread->tid);
        sche_yield(BLOCKED_WAIT);
        lprintf("I am thread %d,  I am back", thread->tid);
        if (wait_node.zombie == NULL) return -1;
        else zombie = wait_node.zombie;
    }

    int ret = zombie->task_id;
    if (status_ptr != NULL) *status_ptr = zombie->status;

    task_destroy(zombie);
    return ret;
}

void kern_halt(void) {
    disable_interrupts();
    printf("Shutdown, Goodbye World!\n");
    sim_halt();
    asm_hlt();
    while (1)
        continue;
}
