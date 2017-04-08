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
#include "vm_internal.h"
#include "asm_page_inval.h"
#include "return_type.h"

//######## DEBUG
#define print_line lprintf("line %d", __LINE__)

uint32_t *kern_page_dir;
uint32_t zfod_frame;

static int num_free_frames;
static mutex_t num_free_frames_mutex;

static uint32_t first_free_frame;
static mutex_t first_free_frame_mutex;

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

        for (j = 0; j < NUM_PT_ENTRIES; j++) {
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

    int machine_frames = machine_phys_frames();
    mutex_init(&first_free_frame_mutex);
    first_free_frame = frame;
    num_free_frames = machine_frames - NUM_KERN_PAGES - 1;
    zfod_frame = PAGE_SIZE * (machine_frames - 1);

    uint32_t last_frame = zfod_frame - PAGE_SIZE;
    while (frame < last_frame) {
        access_physical(frame);
        *((uint32_t *)RW_PHYS_VA) = frame + PAGE_SIZE;
        frame += PAGE_SIZE;
    }
}

// can read from physical page_dir !!!!!!!!!! assumes it's in cr3
uint32_t get_pte(uint32_t addr) {
    uint32_t *page_dir = (uint32_t *)get_cr3();
    int pd_index = PD_INDEX(addr);
    int pt_index = PT_INDEX(addr);

    if (!(page_dir[pd_index] & PTE_PRESENT)) return 0;
    else {
        uint32_t *page_tab = ENTRY_TO_ADDR(page_dir[pd_index]);
        return page_tab[pt_index];
    }
}

void set_pte(uint32_t addr, uint32_t frame_addr, int flags) {
    uint32_t *page_dir = (uint32_t *)get_cr3();
    int pd_index = PD_INDEX(addr);
    int pt_index = PT_INDEX(addr);

    if (!(page_dir[pd_index] & PTE_PRESENT)) {
        page_dir[pd_index] = (uint32_t)smemalign(PAGE_SIZE, PAGE_SIZE);
        page_dir[pd_index] |= PTE_USER | PTE_WRITE | PTE_PRESENT;
    }

    uint32_t *page_tab = ENTRY_TO_ADDR(page_dir[pd_index]);
    page_tab[pt_index] = frame_addr | flags;
}

uint32_t get_frame() {
    mutex_lock(&first_free_frame_mutex);
    uint32_t ret = first_free_frame;
    access_physical(ret);
    first_free_frame = *((uint32_t *)RW_PHYS_VA);
    mutex_unlock(&first_free_frame_mutex);

    memset((void *)RW_PHYS_VA, 0, PAGE_SIZE);
    return ret;
}

void free_frame(uint32_t addr) {
    access_physical(addr);
    
    mutex_lock(&first_free_frame_mutex);
    *((uint32_t *)RW_PHYS_VA) = first_free_frame;
    first_free_frame = addr;
    mutex_unlock(&first_free_frame_mutex);
}

int dec_num_free_frames(int n) {
    int ret = 0;
    mutex_lock(&num_free_frames_mutex);
    if (num_free_frames < n) ret = -1;
    num_free_frames -= n;
    mutex_unlock(&num_free_frames_mutex);
    return ret;
}

void inc_num_free_frames(int n) {
    mutex_lock(&num_free_frames_mutex);
    num_free_frames += n;
    mutex_unlock(&num_free_frames_mutex);
}

int clear_pgdir(uint32_t *pgdir) {
    int i, j;
    for (i = NUM_KERN_TABLES; i < NUM_PD_ENTRIES; i++) {
        uint32_t pde = pgdir[i];
        if ((pde & PDE_PRESENT) == 0) continue;
        uint32_t *pgtable = ENTRY_TO_ADDR(pde);

        for (j = 0; j < NUM_PT_ENTRIES; j++) {
            uint32_t pte = pgtable[j];
            if ((pte & PTE_PRESENT) == 0) continue;

            // don't mess with the RW_PHYS reserved page
            if (i == RW_PHYS_PD_INDEX && j == RW_PHYS_PT_INDEX) continue;
            pgtable[j] = 0;

            uint32_t frame = pte & PAGE_ALIGN_MASK;
            free_frame(frame);
        }

        // don't mess with the RW_PHYS reserved page table
        if (i == RW_PHYS_PD_INDEX) continue;

        sfree(pgtable, PAGE_SIZE);
        pgdir[i] = 0;
    }

    return SUCCESS;
}

int copy_pgdir(uint32_t *new_pgdir, uint32_t *old_pgdir) {
    int i, j;
    for (i = 0; i < NUM_KERN_TABLES; i++) {
        new_pgdir[i] = kern_page_dir[i];
    }

    for (i = NUM_KERN_TABLES; i < NUM_PD_ENTRIES; i++) {
        uint32_t old_pde = old_pgdir[i];
        if ((old_pde & PDE_PRESENT) == 0) continue;
        int new_pde_flag = old_pde & PAGE_FLAG_MASK;
        uint32_t *old_pgtable = ENTRY_TO_ADDR(old_pde);

        uint32_t *new_pgtable = smemalign(PAGE_SIZE, PAGE_SIZE);
        memset(new_pgtable, 0, PAGE_SIZE);
        new_pgdir[i] = (uint32_t)new_pgtable | new_pde_flag;
        // if ((new_pgdir[i] & PDE_PRESENT) == 1) {
        //     lprintf("pgdir %d set successful", i);
        // }
        // print_line;
        // print_line;
        for (j = 0; j < NUM_PT_ENTRIES; j++) {
            uint32_t old_pte = old_pgtable[j];
            if ((old_pte & PTE_PRESENT) == 0) continue;

            // don't mess with the RW_PHYS reserved page
            if (i == RW_PHYS_PD_INDEX && j == RW_PHYS_PT_INDEX) continue;

            // lprintf("read_and_write: i: %d, j: %d", i, j);
            // lprintf("read_and_write, virtual address: %p",
            //         (void *)virtual_addr);
            // print_line;
            // print_line;
            // BUG mutex

            //mutex_lock(&first_free_frame_mutex);
            uint32_t new_physical_frame = get_frame();
            //mutex_unlock(&first_free_frame_mutex);

            // print_line;
            int new_pte_flag = old_pte & PAGE_FLAG_MASK;
            // print_line;
            uint32_t new_pte = new_physical_frame | new_pte_flag;
            // print_line;
            uint32_t virtual_addr = ((uint32_t)i << 22) | ((uint32_t)j << 12);
            write_physical(new_physical_frame, (void *)virtual_addr, PAGE_SIZE);
            // print_line;
            new_pgtable[j] = new_pte;
            // print_line;
        }
    }
    // lprintf("page_dir: %p", new_pgdir);
    // set_cr3((uint32_t)new_pgdir);
    // MAGIC_BREAK;
    // print_line;
    return SUCCESS;
}

// maps RW_PHYS_VA to physical address addr
void access_physical(uint32_t addr) {
    page_inval((void *)RW_PHYS_VA);
    uint32_t entry = addr & PAGE_ALIGN_MASK;
    set_pte(RW_PHYS_VA, entry, PTE_WRITE | PTE_PRESENT);
}

void read_physical(void *virtual_dest, uint32_t phys_src, uint32_t n) {
    access_physical(phys_src);
    uint32_t page_offset = phys_src & ~PAGE_ALIGN_MASK;

    int len;
    if (page_offset + n < PAGE_SIZE) len = n;
    else len = PAGE_SIZE - page_offset;

    uint32_t virtual_src = RW_PHYS_VA + page_offset;
    memcpy(virtual_dest, (void *)virtual_src, len);
}

void write_physical(uint32_t phys_dest, void *virtual_src, uint32_t n) {
    access_physical(phys_dest);
    uint32_t page_offset = (uint32_t)virtual_src & ~PAGE_ALIGN_MASK;

    int len;
    if (page_offset + n < PAGE_SIZE) len = n;
    else len = PAGE_SIZE - page_offset;

    uint32_t virtual_dest = RW_PHYS_VA + page_offset;
    memcpy((void *)virtual_dest, virtual_src, len);
}
