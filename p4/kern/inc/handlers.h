/** @file handlers.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _HANDLERS_H_
#define _HANDLERS_H_

#define FLAG_TRAP_GATE 0x0f
#define FLAG_PL_USER 0x60
#define FLAG_PL_KERNEL 0x00
#define FLAG_PRESENT 0x80

int handler_init();

int exception_init();

int syscall_init();

int device_init();

int pack_idt_high(void *offset, int dpl, int size);

int pack_idt_low(int selector, void *offset);

void idt_install(int idt_idx,
                 void (*entry)(),
                 int selector,
                 int flag);

#endif
