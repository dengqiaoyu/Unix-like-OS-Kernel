/** @file asm_syscalls.h
 *
 */

#ifndef _ASM_SYSCALLS_H_
#define _ASM_SYSCALLS_H_

int asm_fork(void);
void asm_exec(void);
int asm_gettid(void);

#endif
