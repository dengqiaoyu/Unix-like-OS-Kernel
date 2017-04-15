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
#include <console.h>
#include <assert.h>

#include "vm.h"
#include "task.h"
#include "scheduler.h"
#include "mutex.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h"      /* allocator */
#include "tcb_hashtab.h"    /* tcb hash table */
#include "asm_page_inval.h"

extern unsigned int num_ticks;

extern thread_t *init_thread;

extern allocator_t *sche_allocator;

extern uint32_t *kern_page_dir;
extern uint32_t zfod_frame;
extern int num_free_frames;

int kern_exec(void) {
    uint32_t *esi = (uint32_t *)get_esi();
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
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES - 1, 0);
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

int kern_new_pages(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    uint32_t base = (uint32_t)(*esi);
    uint32_t len = (uint32_t)(*(esi + 1));

    if (base & (~PAGE_ALIGN_MASK)) {
        return -1;
    }

    if (len == 0 || len % PAGE_SIZE != 0) {
        return -1;
    }

    uint32_t high = base + (len - 1);
    // check for overflow
    if (high < base) {
        return -1;
    }

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    if (maps_find(task->maps, base, high)) {
        // already mapped or reserved
        return -1;
    }

    if (dec_num_free_frames(len / PAGE_SIZE) < 0) {
        // not enough memory

        // maps_print(task->maps);
        return -1;
    }

    uint32_t offset;
    for (offset = 0; offset < len; offset += PAGE_SIZE) {
        set_pte(base + offset, zfod_frame, PTE_USER | PTE_PRESENT);
    }

    maps_insert(task->maps, base, high, MAP_USER | MAP_WRITE | MAP_REMOVE);

    return 0;
}

int kern_remove_pages(void) {
    uint32_t base = (uint32_t)get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    map_t *map = maps_find(task->maps, base, base);

    if (!map) {
        // already mapped or reserved
        return -1;
    }

    if (map->low != base) {
        return -1;
    }

    if (!(map->perms & MAP_REMOVE)) {
        return -1;
    }

    uint32_t len = map->high - map->low + 1;
    inc_num_free_frames(len / PAGE_SIZE);

    uint32_t addr, frame;
    for (addr = map->low; addr < map->high; addr += PAGE_SIZE) {
        frame = get_pte(addr) & PAGE_ALIGN_MASK;
        assert(frame != 0);
        if (frame != zfod_frame) free_frame(frame);
        page_inval((void *)addr);
        set_pte(addr, 0, 0);
    }

    maps_delete(task->maps, base);

    return 0;
}

int kern_wait(void) {
    int *status_ptr = (int *)get_esi();

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
        sche_yield(BLOCKED_WAIT);

        if (wait_node.zombie == NULL) return -1;
        else zombie = wait_node.zombie;
    }

    int ret = zombie->task_id;
    if (status_ptr != NULL) *status_ptr = zombie->status;

    task_destroy(zombie);
    return ret;
}

void kern_vanish(void) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    mutex_lock(&(task->vanish_mutex));
    task_t *parent = task->parent_task;

    mutex_lock(&(task->thread_list_mutex));
    remove_node(task->live_thread_list, TCB_TO_LIST_NODE(thread));
    int live_threads = get_list_size(task->live_thread_list);
    add_node_to_tail(task->zombie_thread_list, TCB_TO_LIST_NODE(thread));
    // TODO try to free previous zombie stack
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
    }
    else {
        mutex_unlock(&(task->thread_list_mutex));

        orphan_children(task);
        orphan_zombies(task);

        if (parent == NULL) lprintf("init or idle task vanished?");
        mutex_lock(&(parent->wait_mutex));

        mutex_lock(&(parent->child_task_list_mutex));
        // assumes we are in the parent's child task list
        remove_node(parent->child_task_list, TASK_TO_LIST_NODE(task));
        mutex_unlock(&(parent->child_task_list_mutex));

        if (get_list_size(parent->waiting_thread_list) > 0) {
            node_t *node = pop_first_node(parent->waiting_thread_list);
            mutex_unlock(&(parent->wait_mutex));

            wait_node_t *waiter = (wait_node_t *)node;
            waiter->zombie = task;

            // must disable interrupts here
            // otherwise, the waiting thread might free us before we yield
            disable_interrupts();
            waiter->thread->status = RUNNABLE;
            sche_push_back(waiter->thread);

            cli_mutex_unlock(&(task->vanish_mutex));
            sche_yield(ZOMBIE);
        } else {
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
        }
    }

    // TODO error handling
    lprintf("returned from end of vanish");
}

int kern_yield(void) {
    uint32_t *esi = (uint32_t *)get_esi();
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

void kern_set_status(void) {
    int status = (int)get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    task->status = status;
}

unsigned int kern_get_ticks(void) {
    return num_ticks;
}

int kern_sleep(void) {
    int ticks = (int)get_esi();

    if (ticks == 0) return 0;
    unsigned int cur_ticks = num_ticks;
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

int kern_print(void) {
    // TODO BUG
    return 0;

    uint32_t *esi = (uint32_t *)get_esi();
    int len = (int)(*esi);
    char *buf = (char *)(*(esi + 1));

    // TODO macro megabyte
    if (len > 1024 * 1024) return -1;
    int ret = validate_user_mem((uint32_t)buf, len, MAP_USER);
    if (ret < 0) return -1;

    putbytes(buf, len);
    return 0;
}

// TODO put this elsewhere
#define COLOR_MASK 0x7F

int kern_set_term_color(void) {
    int color = (int)get_esi();

    if (color & ~COLOR_MASK) return -1;
    return set_term_color(color);
}

int kern_set_cursor_pos(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    int row = (int)(*esi);
    int col = (int)(*(esi + 1));

    return set_cursor(row, col);
}

int kern_get_cursor_pos(void) {
    uint32_t *esi = (uint32_t *)get_esi();
    int *row = (int *)(*esi);
    int *col = (int *)(*(esi + 1));

    int ret;
    ret = validate_user_mem((uint32_t)row, sizeof(int), MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;
    ret = validate_user_mem((uint32_t)col, sizeof(int), MAP_USER | MAP_WRITE);
    if (ret < 0) return -1;

    get_cursor(row, col);
    return 0;
}
