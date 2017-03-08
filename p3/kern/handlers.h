/** @file handlers.h
 *  @author Newton Xie (ncx)
 *  @bug No known bugs.
 */

#ifndef _HANDLERS_H_
#define _HANDLERS_H_

int install_handlers();

int pack_idt_high(void *offset, int dpl, int size);

int pack_idt_low(int selector, void *offset);

void pf_handler();

void roll_over();

#endif
