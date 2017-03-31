/** @file vm.c
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <simics.h>
#include <page.h>
#include <string.h>
#include <common_kern.h>
#include <x86/cr.h>
#include <mutex.h>

#include "vm.h"
#include "asm_page_inval.h"
#include "return_type.h"

//######## DEBUG
#define print_line lprintf("line %d", __LINE__)

uint32_t *kern_page_dir;
// thread safe?
uint32_t first_free_frame;
mutex_t first_free_frame_mutex;

// maps RW_PHYS_VA to physical address addr
void access_physical(uint32_t addr) {
    page_inval((void *)RW_PHYS_VA);
    uint32_t *page_dir = (uint32_t *)get_cr3();
    uint32_t *pt_va = PTE_TO_ADDR(page_dir[RW_PHYS_PD_INDEX]);
    uint32_t entry = addr & PAGE_MASK;
    pt_va[RW_PHYS_PT_INDEX] = entry | PTE_WRITE | PTE_PRESENT;
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

void vm_init() {
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

    // we need to populate the last entry of the page directory
    // the last page table here will be used for accessing physical addresses
    kern_page_dir[RW_PHYS_PD_INDEX] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE);
    kern_page_dir[RW_PHYS_PD_INDEX] |= flags;

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
    mutex_init(&first_free_frame_mutex);
}

// can read from physical page_dir !!!!!!!!!! assumes it's in cr3
uint32_t get_pte(uint32_t addr) {
    uint32_t *page_dir = (uint32_t *)get_cr3();
    int pd_index = PD_INDEX(addr);
    int pt_index = PT_INDEX(addr);

    if (!(page_dir[pd_index] & PTE_PRESENT)) return 0;
    else {
        uint32_t *page_tab = PTE_TO_ADDR(page_dir[pd_index]);
        return page_tab[pt_index];
    }
}

void set_pte(uint32_t addr, int flags) {
    uint32_t *page_dir = (uint32_t *)get_cr3();
    int pd_index = PD_INDEX(addr);
    int pt_index = PT_INDEX(addr);

    if (!(page_dir[pd_index] & PTE_PRESENT)) {
        page_dir[pd_index] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE);
        page_dir[pd_index] |= PTE_USER | PTE_WRITE | PTE_PRESENT;
    }

    uint32_t *page_tab = PTE_TO_ADDR(page_dir[pd_index]);
    if (!(page_tab[pt_index] & PTE_PRESENT)) {
        page_tab[pt_index] = get_free_frame() | flags;
    } else {
        page_tab[pt_index] &= PAGE_MASK;
        page_tab[pt_index] |= flags;
    }
}

uint32_t get_free_frame() {
    uint32_t ret = first_free_frame;
    access_physical(ret);

    first_free_frame = *((uint32_t *)RW_PHYS_VA);
    memset((void *)RW_PHYS_VA, 0, PAGE_SIZE);
    return ret;
}

int copy_pgdir(uint32_t *new_pgdir, uint32_t *old_pgdir) {
    int i;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        new_pgdir[i] = kern_page_dir[i];
    }

    for (i = NUM_KERN_TABLES; i < NUM_PAGE_DIRECTORY_ENTRY; i++) {
        uint32_t old_pde = old_pgdir[i];
        if ((old_pde & PDE_PRESENT) == 0) continue;
        int new_pde_flag = old_pde & PAGE_FALG_MASK;
        // print_line;
        new_pgdir[i] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE) | new_pde_flag;
        if ((new_pgdir[i] & PDE_PRESENT) == 1) {
            lprintf("pgdir %d set successful", i);
        }
        // print_line;
        uint32_t *old_pgtable = (uint32_t *)(old_pde & PAGE_MASK);
        // print_line;
        uint32_t *new_pgtable = (uint32_t *) (new_pgdir[i] & PAGE_MASK);
        // print_line;
        int j;
        for (j = 0; j < PAGES_PER_TABLE; j++) {
            uint32_t old_pte = old_pgtable[j];
            if ((old_pte & PTE_PRESENT) == 0) continue;
            uint32_t virtual_addr = ((uint32_t)i << 22) | ((uint32_t)j << 12);
            if ((old_pte & PDE_WRITE) == 0) {
                lprintf("ready_only, virtual address: %p",
                        (void *)virtual_addr);
                new_pgtable[j] = old_pte;
                continue;
            }
            // lprintf("read_and_write: i: %d, j: %d", i, j);
            lprintf("read_and_write, virtual address: %p",
                    (void *)virtual_addr);
            // print_line;
            // print_line;
            // BUG mutex
            uint32_t new_physical_frame = get_free_frame();
            // print_line;
            int new_pte_flag = old_pte & PAGE_FALG_MASK;
            // print_line;
            uint32_t new_pte = new_physical_frame | new_pte_flag;
            // print_line;
            write_physical(new_physical_frame, (void *)virtual_addr, PAGE_SIZE);
            // print_line;
            new_pgtable[j] = new_pte;
            // print_line;
        }
    }
    lprintf("page_dir: %p", new_pgdir);
    // set_cr3((uint32_t)new_pgdir);
    // MAGIC_BREAK;
    // print_line;
    return SUCCESS;
}

