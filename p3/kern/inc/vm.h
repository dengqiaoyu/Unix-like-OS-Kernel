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
#define PTE_WRITE (0x2)
#define PTE_USER (0x4)

#define PDE_PRESENT (0x1)
#define PDE_WRITE (0x2)
#define PDE_USER (0x4)

#define CR0_PG (1 << 31)
#define CR4_PGE (1 << 7)

#define RW_PHYS_PD_INDEX 1023
#define RW_PHYS_PT_INDEX 1023
#define RW_PHYS_VA 0xFFFFF000

#define NUM_PD_ENTRIES 1024
#define NUM_PT_ENTRIES 1024
#define NUM_KERN_TABLES 4
#define NUM_KERN_PAGES 4096

int vm_init();

uint32_t get_pte(uint32_t addr);

void set_pte(uint32_t addr, uint32_t frame_addr, int flags);

uint32_t get_frame();

void free_frame(uint32_t frame);

int dec_num_free_frames(int n);

void inc_num_free_frames(int n);

uint32_t *page_dir_init();

int page_dir_clear(uint32_t *page_dir);

int page_dir_copy(uint32_t *new_page_dir, uint32_t *old_page_dir);

void undo_page_dir_copy(uint32_t *page_dir);

uint32_t *get_kern_page_dir(void);

uint32_t get_zfod_frame(void);

#endif
