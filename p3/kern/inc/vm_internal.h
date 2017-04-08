/** @file vm_internal.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _VM_INTERNAL_H_
#define _VM_INTERNAL_H_

#include <stdint.h>

#define PD_INDEX(addr) ((addr >> 22) & 0x3FF)
#define PT_INDEX(addr) ((addr >> 12) & 0x3FF)
#define ENTRY_TO_ADDR(pte) ((void *)(pte & PAGE_ALIGN_MASK))

#define CHECK_ALLOC(addr) if (addr == NULL) lprintf("bad malloc")

void access_physical(uint32_t addr);

void read_physical(void *virtual_dest, uint32_t phys_src, uint32_t n);

void write_physical(uint32_t phys_dest, void *virtual_src, uint32_t n);

#endif
