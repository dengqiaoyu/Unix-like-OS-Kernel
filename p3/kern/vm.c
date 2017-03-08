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

void vm_init()
{
    page_array_size = machine_phys_frames() * sizeof(page_t);
    page_array = malloc(page_array_size);
    // needs checking...
    memset(page_array, 0, page_array_size);

    // kern_page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    kern_page_dir = (void *)0x00010000;
    lprintf("%p\n", kern_page_dir);

    memset(kern_page_dir, 0, PAGE_SIZE);

    int i;
    for (i = 0; i < NUM_KERN_PAGES; i++) {
        page_array[i].used = 1;
    }

    uint32_t *page_addr = 0;
    int flags = PTE_PRESENT | PTE_WRITE;
    int j;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        // void *page_tab_addr = smemalign(PAGE_SIZE, PAGE_SIZE);
        void *page_tab_addr = (void *)(0x00011000 + i * PAGE_SIZE);
        lprintf("%p\n", page_tab_addr);

        kern_page_dir[i] = (uint32_t)page_tab_addr | flags;
        uint32_t *pte_p = page_tab_addr;
        for (j = 0; j < PAGES_PER_TABLE; j++) {
            *pte_p = (uint32_t)page_addr | flags;
            pte_p++;
            page_addr += PAGE_SIZE;
        }
    }

    set_cr3((uint32_t)kern_page_dir);
    lprintf("%p\n", kern_page_dir);
    MAGIC_BREAK;

    set_cr0(get_cr0() | CR0_PG);
    set_cr4(get_cr4() | CR4_PGE);
}

uint32_t *get_pte(uint32_t *page_dir, uint32_t page_va)
{
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    if (!(page_dir[page_dir_index] & PTE_PRESENT)) {
        return NULL;
    }

    uint32_t *pt = (uint32_t *)(page_dir[page_dir_index] & ~PAGE_MASK);
    return &(pt[page_tab_index]);
}

void set_pte(uint32_t *page_dir, uint32_t page_va, uint32_t *page, int flags)
{
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

uint32_t *get_free_page()
{
    int i;
    for (i = 0; i < page_array_size; i++) {
        if (page_array[i].used == 0) {
            page_array[i].used = 1;
            return (uint32_t *)(PAGE_SIZE * i);
        }
    }
    return NULL;
}
