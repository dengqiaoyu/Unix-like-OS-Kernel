/** @file asm_syscalls.h
 *
 */

#ifndef _ASM_SYSCALLS_H_
#define _ASM_SYSCALLS_H_

void asm_fork(void);

void asm_exec(void);

void asm_gettid(void);

void asm_new_pages(void);

#endif
