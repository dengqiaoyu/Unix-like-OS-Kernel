/** @file syscalls.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <page.h>
#include <simics.h>
#include <x86/cr.h>
#include <malloc.h>         /* malloc, smemalign, sfree */
#include <asm.h>            /* disable_interrupts enable_interrupts */

#include "vm.h"
#include "task.h"
#include "scheduler.h"
#include "mutex.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h"      /* allocator */
#include "tcb_hashtab.h"    /* tcb hash table */

extern allocator_t *sche_allocator;

extern uint32_t *kern_page_dir;
extern uint32_t zfod_frame;
extern int num_free_frames;

int kern_exec(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    char *execname = (char *)(*esi);
    char **argvec = (char **)(*(esi + 1));

    // TODO
    // need to check arg counts and lengths
    int argc = 0;
    char **temp = argvec;
    while (*temp) {
        argc += 1;
        temp += 1;
    }

    simple_elf_t elf_header;
    // TODO
    // macro for namebuf length
    // need to check execname length fits
    char namebuf[32];
    sprintf(namebuf, "%s", execname);
    elf_load_helper(&elf_header, namebuf);

    char argbuf[256];
    char *ptrbuf[16];
    char *arg;
    char *marker = argbuf;
    int len;
    int i;
    for (i = 0; i < argc; i++) {
        arg = *(argvec + i);
        len = strlen(arg);
        strncpy(marker, arg, len);
        marker[len] = '\0';
        ptrbuf[i] = marker;
        marker += len + 1;
    }
    ptrbuf[argc] = NULL;

    thread_t *thread = get_cur_tcb();
    thread->cur_sp = USER_STACK_START;
    thread->ip = elf_header.e_entry;

    task_t *task = thread->task;
    maps_clear(task->maps);
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(task->maps, RW_PHYS_VA, PAGE_SIZE, 0);

    page_dir_clear(task->page_dir);
    set_cr3((uint32_t)task->page_dir);

    load_program(&elf_header, task->maps);

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

int kern_wait(void) {
    int *status_ptr = (int *)get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    /*
    if (status_ptr != NULL) {
        map_t *map = maps_find(task->maps, (uint32_t)status_ptr, sizeof(int));
        if (map == NULL) return -1;
        if (!(map->perms & MAP_USER)) return -1;
        if (!(map->perms & MAP_WRITE)) return -1;
    }
    */

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
        mutex_unlock(&(task->wait_mutex));
        add_node_to_tail(task->waiting_thread_list, &(wait_node.node));
        sche_yield(BLOCKED_WAIT);

        if (wait_node.zombie == NULL) return -1;
        else zombie = wait_node.zombie;
    }

    int ret = zombie->task_id;
    if (status_ptr != NULL) *status_ptr = zombie->status;

    task_destroy(zombie);
    return ret;
}

/* yield */

int kern_deschedule(void) {
    uint32_t *esi = (uint32_t *)get_esi();
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
    uint32_t *esi = (uint32_t *)get_esi();
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

int kern_gettid(void) {
    thread_t *thread = get_cur_tcb();
    return thread->tid;
}

int kern_new_pages(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    uint32_t base = (uint32_t)(*esi);
    uint32_t len = (uint32_t)(*(esi + 1));

    if (base & (~PAGE_ALIGN_MASK)) {
        return -1;
    }

    if (len % PAGE_SIZE != 0 || len == 0) {
        return -1;
    }

    // this is an ugly check for "overflowing" len
    if (base + (len - 1) < base) {
        return -1;
    }

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    if (maps_find(task->maps, base, len)) {
        // already mapped or reserved
        return -1;
    }

    if (dec_num_free_frames(len / PAGE_SIZE) < 0) {
        // not enough memory

        // maps_print(task->maps);
        return -1;
    }

    uint32_t page_addr;
    for (page_addr = base; page_addr - base > len; page_addr += PAGE_SIZE) {
        set_pte(page_addr, zfod_frame, PTE_USER | PTE_PRESENT);
    }

    maps_insert(task->maps, base, len, MAP_USER | MAP_WRITE);

    return 0;
}

void kern_set_status(void) {
    int status = (int)get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task->status = status;
}

void kern_vanish(void) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task_t *parent = task->parent_task;

    mutex_lock(&(task->thread_list_mutex));
    remove_node(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    int live_threads = get_list_size(task->live_thread_list);
    add_node_to_head(task->zombie_thread_list, TCB_TO_LIST_NODE(thread));
    mutex_unlock(&(task->thread_list_mutex));

    if (live_threads == 0 && parent != NULL) {
        mutex_lock(&(parent->wait_mutex));

        mutex_lock(&(parent->child_task_list_mutex));
        remove_node(parent->child_task_list, TASK_TO_LIST_NODE(task));
        mutex_unlock(&(parent->child_task_list_mutex));

        if (get_list_size(parent->waiting_thread_list) > 0) {
            wait_node_t *waiter;
            waiter = (wait_node_t *)pop_first_node(parent->waiting_thread_list);
            mutex_unlock(&(parent->wait_mutex));
            waiter->zombie = task;

            // must disable interrupts here
            // otherwise, the waiting thread might free us before we yield
            disable_interrupts();
            waiter->thread->status = RUNNABLE;
            sche_push_back(waiter->thread);
        } else {
            node_t *last_node = get_last_node(parent->zombie_task_list);
            if (last_node != NULL) {
                task_t *prev_zombie = LIST_NODE_TO_TASK(last_node);

                // we can free the previous zombie's vm and thread resources
                page_dir_clear(prev_zombie->page_dir);
                sfree(prev_zombie->page_dir, PAGE_SIZE);
                prev_zombie->page_dir = NULL;

                maps_destroy(prev_zombie->maps);
                prev_zombie->maps = NULL;

                reap_threads(LIST_NODE_TO_TASK(prev_zombie));
            }

            add_node_to_tail(parent->zombie_task_list, TASK_TO_LIST_NODE(task));
            mutex_unlock(&(parent->wait_mutex));
        }
    }

    sche_yield(ZOMBIE);

    lprintf("returned from end of vanish");
    while (1) continue;
}
