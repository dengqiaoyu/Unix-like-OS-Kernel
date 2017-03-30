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

#define RW_PHYS_PD_ENTRY 1022
#define RW_PHYS_VA 0xFFBFF000
#define SELF_REF_PD_ENTRY 1023

#define PD_INDEX(addr) (addr >> 22) & 0x3FF
#define PT_INDEX(addr) (addr >> 12) & 0x3FF
#define PD_VA (void *)0xFFFFF000
#define PT_VA(pd_index) (void *)(0xFFC00000 | (pd_index << 12))

#define CHECK_ALLOC(addr) if (addr == NULL) lprintf("bad malloc")

uint32_t *kern_page_dir;
uint32_t first_free_frame;

// maps RW_PHYS_VA to physical address addr
void access_physical(uint32_t addr) {
    page_inval((void *)RW_PHYS_VA);

    int page_dir_index = PD_INDEX(RW_PHYS_VA);
    int page_tab_index = PT_INDEX(RW_PHYS_VA);
    uint32_t *pt_va = PT_VA(page_dir_index);

    uint32_t entry = addr & PAGE_MASK;
    pt_va[page_tab_index] = entry | PTE_WRITE | PTE_PRESENT;
}

void read_physical(void *virtual_dest, uint32_t phys_src, uint32_t n) {
    access_physical(phys_src);
    uint32_t page_offset = phys_src & ~PAGE_MASK;

    int len;
    if (page_offset + n < PAGE_SIZE) len = n;
    else len = PAGE_SIZE - page_offset;

    uint32_t virtual_src = RW_PHYS_VA + page_offset;
    memcpy(virtual_dest, (void *)virtual_src, len);
}

void write_physical(uint32_t phys_dest, void *virtual_src, uint32_t n) {
    access_physical(phys_dest);
    uint32_t page_offset = (uint32_t)virtual_src & ~PAGE_MASK;

    int len;
    if (page_offset + n < PAGE_SIZE) len = n;
    else len = PAGE_SIZE - page_offset;

    uint32_t virtual_dest = RW_PHYS_VA + page_offset;
    memcpy((void *)virtual_dest, virtual_src, len);
}

void vm_init()
{
    kern_page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    memset(kern_page_dir, 0, PAGE_SIZE);

    uint32_t frame = 0;
    int flags = PTE_WRITE | PTE_PRESENT;

    uint32_t *page_tab_addr;
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

    // the last entry in this page table will be for accessing physical address
    kern_page_dir[RW_PHYS_PD_ENTRY] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE);
    kern_page_dir[RW_PHYS_PD_ENTRY] |= flags;

    // for writing to page tables outside of lowest 16MB
    kern_page_dir[SELF_REF_PD_ENTRY] = (uint32_t)kern_page_dir | flags;

    set_cr3((uint32_t)kern_page_dir);
    set_cr0(get_cr0() | CR0_PG);
    set_cr4(get_cr4() | CR4_PGE);

    first_free_frame = frame;
    uint32_t last_frame = PAGE_SIZE * (machine_phys_frames() - 1);
    while (frame < last_frame) {
        access_physical(frame);
        *((uint32_t *)RW_PHYS_VA) = frame + PAGE_SIZE;
        frame += PAGE_SIZE;
    }
}

// can read from physical page_dir !!!!!!!!!! assumes it's in cr3
uint32_t get_pte(uint32_t addr)
{
    int page_dir_index = PD_INDEX(addr);
    int page_tab_index = PT_INDEX(addr);
    uint32_t *pd_va = PD_VA;
    uint32_t *pt_va = PT_VA(page_dir_index);

    if (!(pd_va[page_dir_index] & PTE_PRESENT)) return 0;
    else return pt_va[page_tab_index];
}

void set_pte(uint32_t addr, int flags)
{
    int page_dir_index = PD_INDEX(addr);
    int page_tab_index = PT_INDEX(addr);
    uint32_t *pd_va = PD_VA;
    uint32_t *pt_va = PT_VA(page_dir_index);

    if (!(pd_va[page_dir_index] & PTE_PRESENT)) {
        pd_va[page_dir_index] = get_free_frame();
        pd_va[page_dir_index] |= PTE_USER | PTE_WRITE | PTE_PRESENT;
    }

    if (!(pt_va[page_tab_index] & PTE_PRESENT)) {
        pt_va[page_tab_index] = get_free_frame() | flags;
    }
    else {
        pt_va[page_tab_index] &= PAGE_MASK;
        pt_va[page_tab_index] |= flags;
    }
}

uint32_t get_free_frame()
{
    uint32_t ret = first_free_frame;
    access_physical(ret);

    first_free_frame = *((uint32_t *)RW_PHYS_VA);
    memset((void *)RW_PHYS_VA, 0, PAGE_SIZE);
    return ret;
}

