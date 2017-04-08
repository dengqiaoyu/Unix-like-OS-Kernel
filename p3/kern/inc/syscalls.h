/** @file syscalls.h
 *
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <page.h>
#include <simics.h>
#include <x86/cr.h>
#include <mutex.h>  /* mutex */
#include <string.h> /* memset */
#include <malloc.h> /* malloc, smemalign, sfree */

#include "vm.h"
#include "scheduler.h"
#include "asm_registers.h"
#include "asm_switch.h"
#include "allocator.h" /* allocator */
#include "asm_set_exec_context.h"

extern sche_node_t *cur_sche_node;
extern uint32_t *kern_page_dir;
extern id_counter_t id_counter;
extern allocator_t *sche_allocator;

int kern_gettid(void);

void kern_exec(void);

int kern_fork(void);

int kern_new_pages(void);

#endif
