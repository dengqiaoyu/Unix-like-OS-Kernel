/** @file asm_exceptions.h
 *
 */

#ifndef _ASM_EXCEPTIONS_H_
#define _ASM_EXCEPTIONS_H_

void asm_divide();

void asm_debug();

void asm_nonmaskable();

void asm_break_point();

void asm_overflow();

void asm_boundcheck();

void asm_opcode();

void asm_nofpu();

void asm_segfault();

void asm_stackfault();

void asm_protfault();

void asm_pagefault();

void asm_fpufault();

void asm_alignfault();

void asm_simdfault();

void asm_coproc();

void asm_task_segf();

void asm_machine_check();

#endif
