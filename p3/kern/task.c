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
#include "task_internal.h"
#include "allocator.h"
#include "scheduler.h"
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
    if (task_node == NULL) return NULL;
    task_t *task = NODE_TO_PCB(task_node);

    task->page_dir = page_dir_init();
    if (task->page_dir == NULL) {
        free(task_node);
        return NULL;
    }

    if (task_lists_init(task) < 0) {
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    if (task_mutexes_init(task) < 0) {
        task_lists_destroy(task);
        sfree(task->page_dir, PAGE_SIZE);
        free(task_node);
        return NULL;
    }

    /*
    task->maps = maps_init();
    if (task->maps == NULL) {
        sfree(task->page_dir, PAGE_SIZE);
        free(task);
        return NULL;
    }
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(task->maps, RW_PHYS_VA, PAGE_SIZE, 0);
    */

    return task;
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

    ret = mutex_init(&(task->live_thread_list_mutex));
    if (ret < 0) return -1;

    ret = mutex_init(&(task->zombie_thread_list_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->live_thread_list_mutex));
        return -1;
    }

    ret = mutex_init(&(task->child_task_list_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->live_thread_list_mutex));
        mutex_destroy(&(task->zombie_thread_list_mutex));
        return -1;
    }

    ret = mutex_init(&(task->wait_mutex));
    if (ret < 0) {
        mutex_destroy(&(task->live_thread_list_mutex));
        mutex_destroy(&(task->zombie_thread_list_mutex));
        mutex_destroy(&(task->child_task_list_mutex));
        return -1;
    }

    return SUCCESS;
}

void task_mutexes_destroy(task_t *task) {
    mutex_destroy(&(task->live_thread_list_mutex));
    mutex_destroy(&(task->zombie_thread_list_mutex));
    mutex_destroy(&(task->child_task_list_mutex));
    mutex_destroy(&(task->wait_mutex));
}

uint32_t *page_dir_init() {
    uint32_t *page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (page_dir == NULL) return NULL;
    memset(page_dir, 0, PAGE_SIZE);

    int i;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        page_dir[i] = kern_page_dir[i];
    }
    return page_dir;
}

// leaves task pointer and status to be set outside
thread_t *thread_init() {
    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    if (sche_node == NULL) {
        // TODO error handling
        return NULL;
    }

    thread_t *thread = GET_TCB(sche_node);
    thread->tid = gen_thread_id();

    void *kern_stack = malloc(KERN_STACK_SIZE);
    if (kern_stack == NULL) {
        allocator_free(sche_node);
        return NULL;
    }
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    thread->cur_sp = USER_STACK_START;

    return thread;
}

// assumes cr3 is already set
int load_program(simple_elf_t *header, map_list_t *maps) {
    lprintf("text");
    load_elf_section(header->e_fname, header->e_txtstart, header->e_txtlen,
                     header->e_txtoff, PTE_USER | PTE_PRESENT);
    maps_insert(maps, header->e_txtstart, header->e_txtlen, MAP_USER);

    lprintf("dat");
    load_elf_section(header->e_fname, header->e_datstart, header->e_datlen,
                     header->e_datoff, PTE_USER | PTE_WRITE | PTE_PRESENT);
    maps_insert(maps, header->e_datstart, header->e_datlen, MAP_USER | MAP_WRITE);

    lprintf("rodat");
    load_elf_section(header->e_fname, header->e_rodatstart, header->e_rodatlen,
                     header->e_rodatoff, PTE_USER | PTE_PRESENT);
    maps_insert(maps, header->e_rodatstart, header->e_rodatlen, MAP_USER);

    lprintf("bss");
    load_elf_section(header->e_fname, header->e_bssstart, header->e_bsslen,
                     -1, PTE_USER | PTE_WRITE | PTE_PRESENT);
    maps_insert(maps, header->e_bssstart, header->e_bsslen, MAP_USER | MAP_WRITE);

    lprintf("stack");
    load_elf_section(header->e_fname, USER_STACK_LOW, USER_STACK_SIZE,
                     -1, PTE_USER | PTE_WRITE | PTE_PRESENT);
    maps_insert(maps, USER_STACK_LOW, USER_STACK_SIZE, MAP_USER | MAP_WRITE);

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
