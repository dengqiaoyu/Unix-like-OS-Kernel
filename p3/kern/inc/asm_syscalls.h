/** @file asm_syscalls.h
 *
 */

#ifndef _ASM_SYSCALLS_H_
#define _ASM_SYSCALLS_H_

void asm_fork(void);

void asm_exec(void);

void asm_wait(void);

void asm_yield(void);

void asm_deschedule(void);

void asm_make_runnable(void);

void asm_gettid(void);

void asm_new_pages(void);

void asm_readline(void);

void asm_set_status(void);

void asm_vanish(void);

#endif
