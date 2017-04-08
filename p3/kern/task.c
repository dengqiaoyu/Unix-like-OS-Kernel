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
#include "return_type.h"

extern uint32_t *kern_page_dir;
extern int num_free_frames;
extern allocator_t *sche_allocator;

id_counter_t id_counter = {0};

int id_counter_init() {
    id_counter.task_id_counter = 0;
    id_counter.thread_id_counter = 0;
    mutex_init(&id_counter.task_id_counter_mutex);
    mutex_init(&id_counter.thread_id_counter_mutex);

    return SUCCESS;
}

task_t *task_init(const char *fname) {
    task_t *task = malloc(sizeof(task_t));
    task->task_id = id_counter.task_id_counter++;
    task->main_thread = thread_init();
    task->main_thread->task = task;
    task->main_thread->status = INITIALIZED;
    task->parent_task = NULL;
    task->thread_cnt = 1;

    task->page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    uint32_t entry = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    task->page_dir[RW_PHYS_PD_INDEX] = entry;
    // set frame areas to 0 ???????
    int i;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        task->page_dir[i] = kern_page_dir[i];
    }

    task->maps = maps_init();
    maps_insert(task->maps, 0, PAGE_SIZE * NUM_KERN_PAGES, 0);
    maps_insert(task->maps, RW_PHYS_VA, PAGE_SIZE, 0);

    simple_elf_t elf_header;
    elf_load_helper(&elf_header, fname);
    task->main_thread->ip = elf_header.e_entry;

    // register new task for simics symbolic debugging
    sim_reg_process(task->page_dir, fname);

    set_cr3((uint32_t)task->page_dir);
    load_program(&elf_header, task->maps);

    return task;
}

thread_t *thread_init() {
    sche_node_t *sche_node = allocator_alloc(sche_allocator);
    lprintf("########sche_node: %p\n", sche_node);
    thread_t *thread = GET_TCB(sche_node);
    lprintf("$$$$$$$$thread: %p\n", thread);
    thread->tid = id_counter.thread_id_counter++;

    void *kern_stack = malloc(KERN_STACK_SIZE);
    lprintf("thread kern stack is %p\n", kern_stack);
    thread->kern_sp = (uint32_t)kern_stack + KERN_STACK_SIZE;
    // because of manipulation?
    thread->user_sp = USER_STACK_START;

    return thread;
}

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
