/** @file hypervisor.h
 */

#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include <elf_410.h>

#define SEGDES_GUEST_CS 0x01cffb000000ffff
#define SEGDES_GUEST_DS 0x01cff2000000ffff

#define SEGSEL_GUEST_CS (SEGSEL_SPARE0 | 0x3)
#define SEGSEL_GUEST_DS (SEGSEL_SPARE1 | 0x3)

#define GUEST_MEM_SIZE (24 * 1024 * 1024)
#define GUEST_FRAMES (GUEST_MEM_SIZE / PAGE_SIZE)

void hypervisor_init();

int guest_init(simple_elf_t *header);

int load_guest_section(const char *fname, unsigned long start,
                       unsigned long len, unsigned long offset);

#endif
