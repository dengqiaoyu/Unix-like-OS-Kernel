/** @file vm.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _VM_H_
#define _VM_H_

#include <stdint.h>

#define PAGE_MASK (0xFFFFFFFF << PAGE_SHIFT)
#define PTE_ALL_FLAG_MASK (0xfff)
#define PTE_PRESENT (0x1)
#define PTE_WRITE (0x2)
#define PTE_USER (0x4)

#define CR0_PG (1 << 31)
#define CR4_PGE (1 << 7)

#define NUM_KERN_PAGES 4096
#define PAGES_PER_TABLE 1024
#define NUM_KERN_TABLES 4

typedef struct page {
    char used;
} page_t;

void vm_init();

uint32_t get_pte(uint32_t page_va);

void set_pte(uint32_t page_va, uint32_t frame_addr, int flags);

uint32_t get_free_frame();

int copy_pgdir(uint32_t *new_pgdir, uint32_t *old_pgdir);

#endif
