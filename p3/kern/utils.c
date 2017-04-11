#include <stdlib.h>
#include <asm.h>
#include "vm.h"
#include "utils.h"

#define KERN_TOP (1<<24)
#define VM_TO_PDE_IDX(addr) (((addr) & 0xFFC00000) >> 22)
#define VM_TO_PTE_IDX(addr) (((addr) & 0x003FF000) >> 12)


int check_ptr_valid(uint32_t ptr, uint32_t *pgdir_ptr, int writable) {
    // Whether in user memory
    if (ptr < KERN_TOP) return -1;
    int pde_idx = VM_TO_PDE_IDX(ptr);
    int pte_idx = VM_TO_PTE_IDX(ptr);

    uint32_t pde = pgdir_ptr[pde_idx];
    uint32_t pde_flag = pde & PAGE_FLAG_MASK;
    uint32_t pde_present = pde_flag & PDE_PRESENT;
    uint32_t pde_write = pde_flag & PDE_WRITE;
    uint32_t pde_user = pde_flag & PDE_USER;

    if (!pde_present) return -1;
    if (!pde_user) return -1;
    if (writable && !pde_write) return -1;

    uint32_t *pgtable_dir = (uint32_t *) (pde & PAGE_ALIGN_MASK);
    uint32_t pte = pgtable_dir[pte_idx];
    uint32_t pte_flag = pte & PAGE_FLAG_MASK;
    uint32_t pte_present = pte_flag & PTE_PRESENT;
    uint32_t pte_write = pte_flag & PTE_WRITE;
    uint32_t pte_user = pte_flag & PTE_USER;

    if (!pte_present) return -1;
    if (!pte_user) return -1;
    if (writable && !pte_write) return -1;

    return 0;
}