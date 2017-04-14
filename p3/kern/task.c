/** @file task.c
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <simics.h>
#include <page.h>
#include <x86/cr.h>

#include "vm.h"
#include "maps.h"
#include "task.h"
#include "allocator.h"
#include "scheduler.h"
#include "tcb_hashtab.h"
#include "return_type.h"

extern uint32_t *kern_page_dir;
extern int num_free_frames;
extern allocator_t *sche_allocator;

static int thread_id_counter;
static mutex_t thread_id_counter_mutex;

int id_counter_init() {
    thread_id_counter = 0;
    int ret = mutex_init(&thread_id_counter_mutex);
    return ret;
}

int gen_thread_id() {
    mutex_lock(&thread_id_counter_mutex);
    int tid = thread_id_counter++;
    mutex_unlock(&thread_id_counter_mutex);
    return tid;
}

task_t *task_init() {
    task_node_t *task_node = malloc(sizeof(task_node_t) + sizeof(task_t));
    // TODO put in allocator here
    if (task_node == NULL) {
        lprintf("f6");
        return NULL;
    }
    task_t *task = LIST_NODE_TO_TASK(task_node);

    task->page_dir = page_dir_init();
    if (task->page_dir == NULL) {
        lprintf("f7");
        free(task_node);
        return NULL;
    }

    if (task_lists_init(task) < 0) {
        lprintf("f8");
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    if (task_mutexes_init(task) < 0) {
        lprintf("f9");
        sfree(task->page_dir, PAGE_SIZE);
        task_lists_destroy(task);
        free(task_node);
        return NULL;
    }

    task->maps = maps_init();
    if (task->maps == NULL) {
        sfree(task->page_dir, PAGE_SIZE);
        task_lists_destroy(task);
        task_mutexes_destroy(task);
        free(task_node);
        return NULL;
    }

    return task;
}

// assumes no active children
void task_destroy(task_t *task) {
    if (task->page_dir != NULL) {
        page_dir_clear(task->page_dir);
        sfree(task->page_dir, PAGE_SIZE);
    }

    if (task->maps != NULL) {
        maps_destroy(task->maps);
    }

    reap_threads(task);

    task_lists_destroy(task);
    task_mutexes_destroy(task);

    free(TASK_TO_LIST_NODE(task));
}

void reap_threads(task_t *task) {
    node_t *thread_node = pop_first_node(task->zombie_thread_list);
    while (thread_node != NULL) {
        thread_destroy(LIST_NODE_TO_TCB(thread_node));
        thread_node = pop_first_node(task->zombie_thread_list);
    }
}

int task_lists_init(task_t *task) {
    task->live_thread_list = list_init();
    if (task->live_thread_list == NULL) return -1;

    task->zombie_thread_list = list_init();
    if (task->zombie_thread_list == NULL) {
        list_destroy(task->live_thread_list);
        return -1;
    }

    task->child_task_list = list_init();
    if (task->child_task_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        return -1;
    }

    task->zombie_task_list = list_init();
    if (task->zombie_task_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        list_destroy(task->child_task_list);
        return -1;
    }

    task->waiting_thread_list = list_init();
    if (task->waiting_thread_list == NULL) {
        list_destroy(task->live_thread_list);
        list_destroy(task->zombie_thread_list);
        list_destroy(task->child_task_list);
        list_destroy(task->zombie_task_list);
        return -1;
    }

    return SUCCESS;
}

void task_lists_destroy(task_t *task) {
    list_destroy(task->live_thread_list);
    list_destroy(task->zombie_thread_list);
    list_destroy(task->child_task_list);
    list_destroy(task->zombie_task_list);
    list_destroy(task->waiting_thread_list);
}

int task_mutexes_init(task_t *task) {
    int ret;

    ret = mutex_init(&(task->thread_list_mutex));
    if (ret < 0) {
        return -1;
    }

    ret = mutex_init(&(task->child_task_list_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->thread_list_mutex));
        return -1;
    }

    ret = mutex_init(&(task->wait_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->thread_list_mutex));
        mutex_destroy(&(task->child_task_list_mutex));
        return -1;
    }

    return SUCCESS;
}

void task_mutexes_destroy(task_t *task) {
    mutex_destroy(&(task->thread_list_mutex));
    mutex_destroy(&(task->child_task_list_mutex));
    mutex_destroy(&(task->wait_mutex));
}

// leaves task pointer and status to be set outside
thread_t *thread_init() {
    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    if (sche_node == NULL) {
        // TODO error handling
        lprintf("f4");
        return NULL;
    }

    thread_t *thread = SCHE_NODE_TO_TCB(sche_node);
    thread->tid = gen_thread_id();

    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        lprintf("f5");
        allocator_free(sche_node);
        return NULL;
    }
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    thread->cur_sp = USER_STACK_START;
    tcb_hashtab_put(thread);
    return thread;
}

void thread_destroy(thread_t *thread) {
    void *kern_stack = (void *)(thread->kern_sp - KERN_STACK_SIZE);
    free(kern_stack);
    allocator_free(TCB_TO_SCHE_NODE(thread));
}

int validate_user_mem(uint32_t addr, uint32_t len, int perms) {
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;

    if (len == 0) return -1;
    uint32_t low = addr;
    uint32_t high = addr + (len - 1);
    // check overflow
    if (high < low) return -1;

    map_t *map = maps_find(task->maps, low, high);
    if (map == NULL) return -1;
    if ((perms & map->perms) != perms) return -1;

    int ret = 0;
    if (low < map->low) {
        ret = validate_user_mem(low, map->low - low, perms);
        if (ret < 0) return -1;
    }
    if (map->high < high) {
        ret = validate_user_mem(map->high + 1, high - map->high, perms);
        if (ret < 0) return -1;
    }

    return 0;
}

// TODO
// returns negative if string goes outside user memory
// returns 0 if string is in valid memory but does not terminate in max_len
// returns length including null terminator otherwise
int validate_user_string(uint32_t addr, int max_len) {
    /*
    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    */

    return 0;
}

// assumes cr3 is already set
int load_program(simple_elf_t *header, map_list_t *maps) {
    uint32_t high;

    lprintf("text");
    load_elf_section(header->e_fname, header->e_txtstart, header->e_txtlen,
                     header->e_txtoff, PTE_USER | PTE_PRESENT);
    if (header->e_txtlen > 0) {
        high = header->e_txtstart + (header->e_txtlen - 1);
        maps_insert(maps, header->e_txtstart, high, MAP_USER);
    }

    lprintf("dat");
    load_elf_section(header->e_fname, header->e_datstart, header->e_datlen,
                     header->e_datoff, PTE_USER | PTE_WRITE | PTE_PRESENT);
    if (header->e_datlen > 0) {
        high = header->e_datstart + (header->e_datlen - 1);
        maps_insert(maps, header->e_datstart, high, MAP_USER | MAP_WRITE);
    }

    lprintf("rodat");
    load_elf_section(header->e_fname, header->e_rodatstart, header->e_rodatlen,
                     header->e_rodatoff, PTE_USER | PTE_PRESENT);
    if (header->e_rodatlen > 0) {
        high = header->e_rodatstart + (header->e_rodatlen - 1);
        maps_insert(maps, header->e_rodatstart, high, MAP_USER);
    }

    lprintf("bss");
    load_elf_section(header->e_fname, header->e_bssstart, header->e_bsslen,
                     -1, PTE_USER | PTE_WRITE | PTE_PRESENT);
    if (header->e_bsslen > 0) {
        high = header->e_bssstart + (header->e_bsslen - 1);
        maps_insert(maps, header->e_bssstart, high, MAP_USER | MAP_WRITE);
    }

    lprintf("stack");
    load_elf_section(header->e_fname, USER_STACK_LOW, USER_STACK_SIZE,
                     -1, PTE_USER | PTE_WRITE | PTE_PRESENT);
    high = USER_STACK_LOW + (USER_STACK_SIZE - 1);
    maps_insert(maps, USER_STACK_LOW, high, MAP_USER | MAP_WRITE);

    // test maps
    maps_print(maps);

    return 0;
}

int load_elf_section(const char *fname, unsigned long start, unsigned long len,
                     long offset, int pte_flags) {
    lprintf("%p", (void *)start);
    lprintf("%p", (void *)len);
    lprintf("%p", (void *)offset);

    uint32_t low = (uint32_t)start & PAGE_ALIGN_MASK;
    uint32_t high = (uint32_t)(start + len);

    uint32_t addr = low;
    while (addr < high) {
        if (!(get_pte(addr) & PTE_PRESENT)) {
            // TODO fix error handling here
            if (dec_num_free_frames(1) < 0) {
                return -1;
            }

            uint32_t frame_addr = get_frame();
            set_pte(addr, frame_addr, pte_flags);
        }
        addr += PAGE_SIZE;
    }

    if (offset != -1) {
        getbytes(fname, offset, len, (char *)start);
    }

    return 0;
}
