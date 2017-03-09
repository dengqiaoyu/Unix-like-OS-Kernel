/** @file handlers.c
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <simics.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/page.h>
#include <x86/cr.h>
#include <x86/idt.h>

#include "handlers.h"
#include "asm_handlers.h"
#include "vm.h"
#include "task.h"

extern uint32_t *kern_page_dir;

void pf_handler()
{
    uint32_t pf_addr = get_cr2();
    uint32_t *page_dir = (uint32_t *)get_cr3();
    uint32_t pte = get_pte(page_dir, pf_addr);

    MAGIC_BREAK;

    if (!(pte & PTE_PRESENT)) {
        uint32_t *page = get_free_page();
        int flags = PTE_WRITE | PTE_USER;
        set_pte(page_dir, pf_addr, page, flags);
    }
    else {
        MAGIC_BREAK;
        roll_over();
    }

    return;
}

/** @brief Packs high bits of IDT entry
 *
 *  @param offset handler address
 *  @param dpl privilege level
 *  @size size parameter
 *  @return packed high bits
 **/
int pack_idt_high(void *offset, int dpl, int size)
{
    int packed = (int)offset & 0xffff0000;
    packed |= 1 << 15;
    packed |= dpl << 13;
    packed |= size << 11;
    packed |= 3 << 9;
    return packed;
}

/** @brief Packs low bits of IDT entry
 *
 *  @param selector segment selector
 *  @param offset handler address
 *  @return packed low bits
 **/
int pack_idt_low(int selector, void *offset)
{
    int packed = (int)offset & 0xffff;
    packed |= selector << 16;
    return packed;
}

/** @brief
 *
 *  @param
 *  @return
 */
int install_handlers()
{
    uint32_t *pf_idt = (uint32_t *)idt_base() + 2 * IDT_PF;
    *pf_idt = pack_idt_low(SEGSEL_KERNEL_CS, (void *)asm_pf_handler);
    *(pf_idt+1) = pack_idt_high((void *)asm_pf_handler, 0, 1);

    return 0;
}

void roll_over()
{
    lprintf("feeLs bad man\n");
    while (1) continue;
}
