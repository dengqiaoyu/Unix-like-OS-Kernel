/** @file vm.c
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <simics.h>
#include <page.h>
#include <common_kern.h>
#include <x86/cr.h>


#include "vm.h"
#include "malloc_internal.h"

page_t *page_array;
int page_array_size;

uint32_t *kern_page_dir;

void vm_init() {
    int frame_size = machine_phys_frames();
    int kern_page_dir_size = (frame_size + 1023) / 1024;
    uint32_t *kern_page_dir_addr =
        _memalign(PAGE_SIZE, kern_page_dir_size);

    // needs checking...
    // memset(page_array, 0, page_array_size);

    kern_page_dir = _memalign(PAGE_SIZE, 1024);
    // kern_page_dir = (void *)0x00010000;
    lprintf("%p\n", kern_page_dir);

    int flags = PTE_PRESENT | PTE_WRITE;
    int i, j;
    for (i = 0; i < kern_page_dir_size; i++) {

        // lprintf("%p\n", page_tab_addr);
        uint32_t *page_table_entry = kern_page_dir_addr;
        kern_page_dir[i] = (uint32_t)page_table_entry | flags;
        uint32_t *pte_p = kern_page_dir_addr;
        int cnt = 0;
        for (j = 0; j < 1024 && cnt < frame_size; j++, cnt++) {
            pte_p[j] = (uint32_t)(cnt * PAGE_SIZE) | flags;
        }
        kern_page_dir_addr += PAGE_SIZE;
    }

    set_cr3((uint32_t)kern_page_dir);
    // lprintf("%p\n", kern_page_dir);
    // MAGIC_BREAK;

    set_cr0(get_cr0() | CR0_PG);
    // set_cr4(get_cr4() | CR4_PGE);
}

uint32_t *get_pte(uint32_t *page_dir, uint32_t page_va) {
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    if (!(page_dir[page_dir_index] & PTE_PRESENT)) {
        return NULL;
    }

    uint32_t *pt = (uint32_t *)(page_dir[page_dir_index] & ~PAGE_MASK);
    return &(pt[page_tab_index]);
}

void set_pte(uint32_t *page_dir, uint32_t page_va, uint32_t *page, int flags) {
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    if (!(page_dir[page_dir_index] & PTE_PRESENT)) {
        uint32_t *new_page = smemalign(PAGE_SIZE, PAGE_SIZE);
        memset(new_page, 0, PAGE_SIZE);

        // BUG what if not user page?
        int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
        page_dir[page_dir_index] = (uint32_t)new_page | flags;
    }

    uint32_t *pt = (uint32_t *)(page_dir[page_dir_index] & ~PAGE_MASK);
    pt[page_tab_index] = (uint32_t)page | flags;
}

uint32_t *get_free_page() {
    int i;
    for (i = 0; i < page_array_size; i++) {
        if (page_array[i].used == 0) {
            page_array[i].used = 1;
            return (uint32_t *)(PAGE_SIZE * i);
        }
    }
    return NULL;
}
