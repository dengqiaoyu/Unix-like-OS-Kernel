/** @file vm.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _VM_H_
#define _VM_H_

#include <stdint.h>

#define PAGE_ALIGN_MASK (0xFFFFF000)
#define PAGE_FLAG_MASK (~PAGE_ALIGN_MASK)

#define PTE_PRESENT (0x1)
#define PDE_PRESENT (PTE_PRESENT)
#define PTE_WRITE (0x2)
#define PDE_WRITE (PTE_WRITE)
#define PTE_USER (0x4)

#define CR0_PG (1 << 31)
#define CR4_PGE (1 << 7)

#define RW_PHYS_PD_INDEX 1023
#define RW_PHYS_PT_INDEX 1023
#define RW_PHYS_VA 0xFFFFF000

#define NUM_PD_ENTRIES 1024
#define NUM_PT_ENTRIES 1024
#define NUM_KERN_TABLES 4
#define NUM_KERN_PAGES 4096

void vm_init();

uint32_t get_pte(uint32_t addr);

void set_pte(uint32_t addr, int flags);

uint32_t get_frame();

void free_frame(uint32_t addr);

int clear_pgdir(uint32_t *pgdir);

int copy_pgdir(uint32_t *new_pgdir, uint32_t *old_pgdir);

#endif
