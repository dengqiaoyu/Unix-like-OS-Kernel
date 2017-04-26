/**
 * @file memory_management
 * @brief This file contains new_pages and remove_pages to ask for more memory
 * @author Newton Xie (ncx)
 * @author Qiaoyu Deng
 * @bug No known bugs
 */
#include <stdlib.h>
#include <assert.h>

#include "syscalls/syscalls.h"
#include "task.h"
#include "vm.h"
#include "scheduler.h"
#include "asm_page_inval.h"

/**
 * @brief   Allocates new memory to the invoking task, starting at base and
 *          extending for len bytes
 * @return  0 as success, -1 as failure
 */
int kern_new_pages(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    /* read parameters from esi */
    uint32_t base = (*esi);
    uint32_t len = (*(esi + 1));

    /* check base alignment */
    if (base & (~PAGE_ALIGN_MASK)) {
        return -1;
    }

    /* check length requirement */
    if (len == 0 || len % PAGE_SIZE != 0) {
        return -1;
    }

    /* check whether overflow */
    uint32_t high = base + (len - 1);
    if (high < base) {
        return -1;
    }

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    if (maps_find(task->maps, base, high)) {
        /* already mapped or reserved */
        return -1;
    }

    if (dec_num_free_frames(len / PAGE_SIZE) < 0) {
        /* not enough memory */
        return -1;
    }

    uint32_t zfod_frame = get_zfod_frame();

    int ret;
    uint32_t offset;
    /* set page frame for page table entry region from base to base + len */
    for (offset = 0; offset < len; offset += PAGE_SIZE) {
        /* ser page table entry for user */
        ret = set_pte(base + offset, zfod_frame, PTE_USER | PTE_PRESENT);
        if (ret < 0) {
            // set_pte could fail when calling smemalign, so we reset
            uint32_t reoffset;
            for (reoffset = 0; reoffset < offset; reoffset += PAGE_SIZE) {
                // this call will not fail
                set_pte(base + reoffset, 0, 0);
            }
            inc_num_free_frames(len / PAGE_SIZE);
            return -1;
        }
    }

    /* map memory region from base to base + len */
    maps_insert(task->maps, base, high, MAP_USER | MAP_WRITE | MAP_REMOVE);

    return 0;
}

/**
 * @brief   Deallocates the specified memory region, which must presently be
 *          allocated as the result of a previous call to new pages() which
 *          specified the same value of base.
 * @return  0 as success, -1 as failure
 */
int kern_remove_pages(void) {
    uint32_t base = (uint32_t)asm_get_esi();

    thread_t *thread = get_cur_tcb();
    task_t *task = thread->task;
    map_t *map = maps_find(task->maps, base, base);

    /* already mapped or reserved */
    if (!map) return -1;

    /* not align */
    if (map->low != base) return -1;

    /* cannot be removed */
    if (!(map->perms & MAP_REMOVE)) return -1;

    uint32_t len = map->high - map->low + 1;
    inc_num_free_frames(len / PAGE_SIZE);

    uint32_t addr, frame;
    /* free memory frames to kernel and reset the page table entry */
    for (addr = map->low; addr < map->high; addr += PAGE_SIZE) {
        frame = get_pte(addr) & PAGE_ALIGN_MASK;
        assert(frame != 0);
        if (frame != get_zfod_frame()) free_frame(frame);
        inc_num_free_frames(1);
        asm_page_inval((void *)addr);
        set_pte(addr, 0, 0);
    }

    /* delete the mapping */
    maps_delete(task->maps, base);

    return 0;
}

