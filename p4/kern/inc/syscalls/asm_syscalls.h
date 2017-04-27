/** @file asm_syscalls.h
 *
 */

#ifndef _ASM_SYSCALLS_H_
#define _ASM_SYSCALLS_H_

#include <stdlib.h>

void asm_fork(void);

void asm_exec(void);

void asm_wait(void);

void asm_yield(void);

void asm_deschedule(void);

void asm_make_runnable(void);

void asm_gettid(void);

void asm_new_pages(void);

void asm_remove_pages(void);

void asm_sleep(void);

void asm_getchar(void);

void asm_readline(void);

void asm_print(void);

void asm_set_term_color(void);

void asm_set_cursor_pos(void);

void asm_get_cursor_pos(void);

void asm_thread_fork(void);

void asm_get_ticks(void);

void asm_halt(void);

void asm_set_status(void);

void asm_vanish(void);

void asm_readfile(void);

void asm_swexn(void);

/* syscall helper function */
uint32_t asm_get_esi();

#endif
