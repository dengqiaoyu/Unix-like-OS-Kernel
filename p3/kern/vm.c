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

page_t *page_array;
int page_array_size;

uint32_t *kern_page_dir;

void vm_init() {
    int sys_frames = machine_phys_frames();
    page_array_size = sys_frames * sizeof(page_t);
    page_array = malloc(page_array_size);
    // needs checking...
    memset(page_array, 0, page_array_size);

    kern_page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    memset(kern_page_dir, 0, PAGE_SIZE);

    int i;
    for (i = 0; i < NUM_KERN_PAGES; i++) {
        page_array[i].used = 1;
    }

    // for testing
    for (; i < 2 * NUM_KERN_PAGES; i++) {
        page_array[i].used = 1;
    }

    uint32_t page_addr = 0;
    // Do we need to set CR4_PGE ?
    int flags = PTE_PRESENT | PTE_WRITE;
    int j;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        void *page_tab_addr = smemalign(PAGE_SIZE, PAGE_SIZE);
        memset(page_tab_addr, 0, PAGE_SIZE);

        kern_page_dir[i] = (uint32_t)page_tab_addr | flags;
        uint32_t *pte_p = page_tab_addr;
        for (j = 0; j < PAGES_PER_TABLE; j++) {
            *pte_p = page_addr | flags;
            pte_p++;
            page_addr += PAGE_SIZE;
        }
    }

    // for writing
    kern_page_dir[1023] = (uint32_t)kern_page_dir | flags;

    set_cr3((uint32_t)kern_page_dir);
    set_cr0(get_cr0() | CR0_PG);
    set_cr4(get_cr4() | CR4_PGE);
}

// takes physical page_dir !!!!!!!!!! assumes it's in cr3
uint32_t get_pte(uint32_t *page_dir, uint32_t page_va) {
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    int flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    uint32_t *pd_va = (uint32_t *)0xFFFFF000;
    uint32_t *pt_va = (uint32_t *)(0xFFC00000 | (page_dir_index << PAGE_SHIFT));
    if (!(pd_va[page_dir_index] & PTE_PRESENT)) {
        uint32_t *new_page = get_free_page();
        pd_va[page_dir_index] = (uint32_t)new_page | flags;
        memset(pt_va, 0, PAGE_SIZE);
    }

    return pt_va[page_tab_index];
}

void set_pte(uint32_t *page_dir, uint32_t page_va, uint32_t *page, int flags) {
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    uint32_t *pd_va = (uint32_t *)0xFFFFF000;
    uint32_t *pt_va = (uint32_t *)(0xFFC00000 | (page_dir_index << PAGE_SHIFT));
    if (!(pd_va[page_dir_index] & PTE_PRESENT)) {
        uint32_t *new_page = get_free_page();
        pd_va[page_dir_index] = (uint32_t)new_page | flags | PTE_PRESENT;
        memset(pt_va, 0, PAGE_SIZE);
    }

    pt_va[page_tab_index] = (uint32_t)page | flags | PTE_PRESENT;
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
