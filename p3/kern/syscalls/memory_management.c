#include <stdlib.h>
#include <assert.h>

#include "syscalls/syscalls.h"
#include "task.h"
#include "vm.h"
#include "scheduler.h"
#include "asm_page_inval.h"

int kern_new_pages(void) {
    uint32_t *esi = (uint32_t *)asm_get_esi();
    uint32_t base = (*esi);
    uint32_t len = (*(esi + 1));

    if (base & (~PAGE_ALIGN_MASK)) {
        return -1;
    }

    if (len == 0 || len % PAGE_SIZE != 0) {
        return -1;
    }

    uint32_t high = base + (len - 1);
    if (high < base) {
        // overflow
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
        return -1;
    }

    uint32_t zfod_frame = get_zfod_frame();

    int ret;
    uint32_t offset;
    for (offset = 0; offset < len; offset += PAGE_SIZE) {
        ret = set_pte(base + offset, zfod_frame, PTE_USER | PTE_PRESENT);
        if (ret < 0) {
            // set_pte could fail when calling smemalign
            // we need to reset
            uint32_t reoffset;
            for (reoffset = 0; reoffset <= offset; reoffset += PAGE_SIZE) {
                // this one should not fail
                ret = set_pte(base + reoffset, 0, 0);
                assert(ret == 0);
            }
            return -1;
        }
    }

    maps_insert(task->maps, base, high, MAP_USER | MAP_WRITE | MAP_REMOVE);

    return 0;
}

int kern_remove_pages(void) {
    uint32_t base = (uint32_t)asm_get_esi();

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
        if (frame != get_zfod_frame()) free_frame(frame);
        inc_num_free_frames(1);
        asm_page_inval((void *)addr);
        set_pte(addr, 0, 0);
    }

    maps_delete(task->maps, base);

    return 0;
}

