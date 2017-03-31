/** @file vm.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _VM_H_
#define _VM_H_

#include <stdint.h>

#define PAGE_MASK (0xFFFFF000)
#define PAGE_FALG_MASK (~PAGE_MASK)
#define PTE_PRESENT (0x1)
#define PDE_PRESENT (PTE_PRESENT)
#define PTE_WRITE (0x2)
#define PDE_WRITE (PTE_WRITE)
#define PTE_USER (0x4)

#define CR0_PG (1 << 31)
#define CR4_PGE (1 << 7)

#define NUM_KERN_PAGES 4096
#define PAGES_PER_TABLE 1024
// TODO Do we need to leave last page directory void?
#define NUM_PAGE_DIRECTORY_ENTRY (1024 - 1)
#define NUM_KERN_TABLES 4

#define RW_PHYS_PD_INDEX 1023
#define RW_PHYS_PT_INDEX 1023
#define RW_PHYS_VA 0xFFFFF000

#define PD_INDEX(addr) ((addr >> 22) & 0x3FF)
#define PT_INDEX(addr) ((addr >> 12) & 0x3FF)
#define PTE_TO_ADDR(pte) ((void *)(pte & PAGE_MASK))

#define CHECK_ALLOC(addr) if (addr == NULL) lprintf("bad malloc")


void vm_init();

uint32_t get_pte(uint32_t addr);

void set_pte(uint32_t addr, int flags);

uint32_t get_free_frame();

int copy_pgdir(uint32_t *new_pgdir, uint32_t *old_pgdir);

#endif
