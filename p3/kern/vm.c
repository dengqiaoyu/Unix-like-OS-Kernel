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
#include "asm_page_inval.h"

#define NEXT_FREE_FRAME(frame) (*((uint32_t *)frame))

uint32_t *kern_page_dir;
uint32_t first_free_frame;

uint32_t read_physical(uint32_t addr) {
    set_pte(0xFFBFF000, addr, 0);
    /*
    uint32_t *temp = (uint32_t *)(kern_page_dir[1022] & ~0xFFF);
    temp[1023] = addr | PTE_WRITE | PTE_PRESENT;
    */
    page_inval((void *)0xFFBFF000);
    return *((uint32_t *)0xFFBFF000);
}

void write_physical(uint32_t addr, uint32_t val) {
    set_pte(0xFFBFF000, addr, PTE_WRITE);
    /*
    uint32_t *temp = (uint32_t *)(kern_page_dir[1022] & ~0xFFF);
    temp[1023] = addr | PTE_WRITE | PTE_PRESENT;
    */
    page_inval((void *)0xFFBFF000);
    *((uint32_t *)0xFFBFF000) = val;
}

void vm_init()
{
    uint32_t sys_frames = machine_phys_frames();

    // needs checking...
    kern_page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    memset(kern_page_dir, 0, PAGE_SIZE);

    uint32_t frame = 0;

    uint32_t *page_tab_addr;
    int flags = PTE_PRESENT | PTE_WRITE;
    int i, j;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        page_tab_addr = smemalign(PAGE_SIZE, PAGE_SIZE);
        memset(page_tab_addr, 0, PAGE_SIZE);
        kern_page_dir[i] = (uint32_t)page_tab_addr | flags;

        for (j = 0; j < PAGES_PER_TABLE; j++) {
            page_tab_addr[j] = frame | flags;
            frame += PAGE_SIZE;
        }
    }

    // for bad things
    kern_page_dir[1022] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | flags;
    // for writing to page tables outside of lowest 16MB
    kern_page_dir[1023] = (uint32_t)kern_page_dir | flags;

    set_cr3((uint32_t)kern_page_dir);
    set_cr0(get_cr0() | CR0_PG);
    set_cr4(get_cr4() | CR4_PGE);

    first_free_frame = frame;
    uint32_t last_frame = PAGE_SIZE * (sys_frames - 1);
    while (frame < last_frame) {
        write_physical(frame, frame + PAGE_SIZE);
        frame += PAGE_SIZE;
    }
}

// can read from physical page_dir !!!!!!!!!! assumes it's in cr3
uint32_t get_pte(uint32_t page_va)
{
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    uint32_t *pd_va = (uint32_t *)0xFFFFF000;
    uint32_t *pt_va = (uint32_t *)(0xFFC00000 | (page_dir_index << PAGE_SHIFT));
    if (!(pd_va[page_dir_index] & PTE_PRESENT)) {
        uint32_t new_frame = get_free_frame();
        pd_va[page_dir_index] = new_frame | PTE_PRESENT | PTE_WRITE | PTE_USER;
        memset(pt_va, 0, PAGE_SIZE);
    }

    return pt_va[page_tab_index];
}

// calling get_free_frame in two places is nasty
void set_pte(uint32_t page_va, uint32_t frame, int flags)
{
    // can macro these
    int page_dir_index = (page_va >> 22) & 0x3FF;
    int page_tab_index = (page_va >> PAGE_SHIFT) & 0x3FF;

    uint32_t *pd_va = (uint32_t *)0xFFFFF000;
    uint32_t *pt_va = (uint32_t *)(0xFFC00000 | (page_dir_index << PAGE_SHIFT));
    if (!(pd_va[page_dir_index] & PTE_PRESENT)) {
        uint32_t new_frame = get_free_frame();
        pd_va[page_dir_index] = new_frame | flags | PTE_PRESENT;
        memset(pt_va, 0, PAGE_SIZE);
    }

    pt_va[page_tab_index] = frame | flags | PTE_PRESENT;
}

uint32_t get_free_frame()
{
    uint32_t ret = first_free_frame;
    first_free_frame = read_physical(first_free_frame);
    return ret;
}
